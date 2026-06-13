/*
* Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include "dpu_multi_core.h"

#define __CLASS__ "DPUMultiCore"
#define zero_index 0

namespace sdm {

DPUMultiCore::DPUMultiCore(DisplayId display_id, SDMDisplayType type,
                           MultiCoreInstance<uint32_t, HWInfoInterface *> hw_info_intf,
                           BufferAllocator *buffer_allocator)
    : display_id_(display_id),
      type_(type),
      hw_info_intf_(hw_info_intf),
      buffer_allocator_(buffer_allocator) {}

DisplayError DPUMultiCore::Init() {
  DisplayError error = kErrorNone;
  for (auto intf = hw_info_intf_.Begin(); intf != hw_info_intf_.End(); intf++) {
    HWInterface *hw = nullptr;
    uint32_t core_id = intf->first;
    error = HWInterface::Create(display_id_.GetConnId(core_id), type_, intf->second,
                                buffer_allocator_, &hw);
    if (error != kErrorNone) {
      DLOGE("HW interface create failed");
      return error;
    }
    hw_intf_.insert(std::make_pair(core_id, hw));
    core_ids_.push_back(core_id);
  }

  return kErrorNone;
}

DisplayError DPUMultiCore::Destroy() {
  for (auto hw_intf : hw_intf_) {
    HWInterface::Destroy(hw_intf.second);
  }
  return kErrorNone;
}

DisplayError DPUMultiCore::GetDisplayId(int32_t *display_id) {
  *display_id = 0;
  DisplayError error = kErrorNone;
  for (auto hw_intf : hw_intf_) {
    int32_t disp_id;
    error = hw_intf.second->GetDisplayId(&disp_id);
    if (error != kErrorNone) {
      return error;
    }
    *display_id = *display_id | DisplayId(hw_intf.first, disp_id).GetDisplayId();
  }

  return error;
}

DisplayError DPUMultiCore::GetActiveConfig(uint32_t *active_config) {
  std::vector<uint32_t> active_config_list;
  DisplayError error = kErrorNone;
  uint32_t active_config_val = 0;

  for (auto hw_intf : hw_intf_) {
    error = hw_intf.second->GetActiveConfig(&active_config_val);
    if (error != kErrorNone) {
      return error;
    }

    active_config_list.push_back(active_config_val);
  }

  if (!AreAllEntriesSame<uint32_t>(active_config_list)) {
    return kErrorUndefined;
  }
  *active_config = active_config_list[zero_index];

  return kErrorNone;
}

DisplayError DPUMultiCore::GetDefaultConfig(uint32_t *default_config) {
  std::vector<uint32_t> default_config_list;
  DisplayError error = kErrorNone;
  uint32_t default_config_val = 0;

  for (auto hw_intf : hw_intf_) {
    error = hw_intf.second->GetDefaultConfig(&default_config_val);
    if (error != kErrorNone) {
      return error;
    }

    default_config_list.push_back(default_config_val);
  }

  if (!AreAllEntriesSame<uint32_t>(default_config_list)) {
    return kErrorUndefined;
  }
  *default_config = default_config_list[zero_index];

  return kErrorNone;
}

DisplayError DPUMultiCore::GetNumDisplayAttributes(uint32_t *count) {
  std::vector<uint32_t> count_list;
  uint32_t count_val = 0;

  for (auto hw_intf : hw_intf_) {
    DisplayError error = hw_intf.second->GetNumDisplayAttributes(&count_val);
    if (error != kErrorNone) {
      return error;
    }

    count_list.push_back(count_val);
  }

  if (!AreAllEntriesSame<uint32_t>(count_list)) {
    return kErrorUndefined;
  }
  *count = count_list[zero_index];

  return kErrorNone;
}

DisplayError DPUMultiCore::GetDisplayAttributes(uint32_t index, DisplayDeviceContext *device_ctx,
                                                DisplayClientContext *client_ctx) {
  DisplayError error = kErrorNone;
  std::map<uint32_t, HWDisplayAttributes> display_attr_map;

  for (auto hw_intf : hw_intf_) {
    error = hw_intf.second->GetDisplayAttributes(index, &display_attr_map[hw_intf.first]);
    if (error != kErrorNone) {
      return error;
    }
  }

  client_ctx->display_attributes = display_attr_map[core_ids_[0]];
  DisplayInfoContext info_ctx = {};
  info_ctx.display_attributes = display_attr_map[core_ids_[0]];
  if (device_ctx->find(core_ids_[0]) == device_ctx->end()) {
    device_ctx->insert({core_ids_[0], info_ctx});
  } else {
    device_ctx->at(core_ids_[0]).display_attributes = info_ctx.display_attributes;
  }

  auto i = display_attr_map.begin();
  i++;
  for (; i != display_attr_map.end(); i++) {
    auto itr = device_ctx->find(i->first);
    if (itr == device_ctx->end()) {
      DisplayInfoContext info_ctx = {};
      info_ctx.display_attributes = i->second;
      device_ctx->insert({i->first, info_ctx});
    } else {
      itr->second.display_attributes = i->second;
    }
    client_ctx->display_attributes.x_pixels += i->second.x_pixels;
    client_ctx->display_attributes.h_total += i->second.h_total;
  }

  return kErrorNone;
}

void DPUMultiCore::SetOpSyncHint(bool dpu_ctl_op_sync) {
  dpu_ctl_op_sync_ = dpu_ctl_op_sync;

  op_sync_sequence_.resize(hw_intf_.size());
  if (dpu_ctl_op_sync_) {
    std::iota(std::rbegin(op_sync_sequence_), std::rend(op_sync_sequence_), 0);
  } else {
    std::iota(std::begin(op_sync_sequence_), std::end(op_sync_sequence_), 0);
  }
}

DisplayError DPUMultiCore::GetHWPanelInfo(DisplayDeviceContext *device_ctx,
                                          DisplayClientContext *client_ctx) {
  DisplayError error = kErrorNone;
  std::map<uint32_t, HWPanelInfo> panel_info_map;

  for (auto hw_intf : hw_intf_) {
    error = hw_intf.second->GetHWPanelInfo(&panel_info_map[hw_intf.first]);
    if (error != kErrorNone) {
      return error;
    }
  }

  client_ctx->hw_panel_info = panel_info_map[core_ids_[0]];
  DisplayInfoContext info_ctx = {};
  info_ctx.hw_panel_info = panel_info_map[core_ids_[0]];
  if (device_ctx->find(core_ids_[0]) == device_ctx->end()) {
    device_ctx->insert({core_ids_[0], info_ctx});
  } else {
    device_ctx->at(core_ids_[0]).hw_panel_info = info_ctx.hw_panel_info;
  }

  auto i = panel_info_map.begin();
  i++;
  for (; i != panel_info_map.end(); i++) {
    auto itr = device_ctx->find(i->first);
    if (itr == device_ctx->end()) {
      DisplayInfoContext info_ctx = {};
      info_ctx.hw_panel_info = i->second;
      device_ctx->insert({i->first, info_ctx});
    } else {
      itr->second.hw_panel_info = i->second;
    }
    client_ctx->hw_panel_info.min_roi_width += i->second.min_roi_width;
    client_ctx->hw_panel_info.split_info.left_split += i->second.split_info.left_split;
  }

  SetOpSyncHint(panel_info_map[zero_index].dpu_ctl_op_sync);

  return kErrorNone;
}

DisplayError DPUMultiCore::SetDisplayAttributes(uint32_t index) {
  for (auto hw_intf : hw_intf_) {
    DisplayError error = hw_intf.second->SetDisplayAttributes(index);
    if (error != kErrorNone) {
      return error;
    }
  }

  return kErrorNone;
}

DisplayError DPUMultiCore::SetDisplayAttributes(const HWDisplayAttributes &display_attributes) {
  for (auto hw_intf : hw_intf_) {
    DisplayError error = hw_intf.second->SetDisplayAttributes(display_attributes);
    if (error != kErrorNone) {
      return error;
    }
  }

  return kErrorNone;
}

DisplayError DPUMultiCore::GetConfigIndex(char *mode, uint32_t *index) {
  std::vector<uint32_t> index_list;
  uint32_t index_val;

  for (auto hw_intf : hw_intf_) {
    DisplayError error = hw_intf.second->GetConfigIndex(mode, &index_val);
    if (error != kErrorNone) {
      return error;
    }

    index_list.push_back(index_val);
  }

  if (!AreAllEntriesSame<uint32_t>(index_list)) {
    return kErrorUndefined;
  }
  *index = index_list[zero_index];

  return kErrorNone;
}

DisplayError DPUMultiCore::PowerOn(std::map<uint32_t, HWQosData> &qos_data,
                                   SyncPoints *sync_points) {
  std::vector<SyncPoints> sync_points_list;
  SyncPoints sync_points_val;
  DisplayError error = kErrorNone;

  for (uint32_t i : op_sync_sequence_) {
    error = hw_intf_.at(core_ids_[i])->PowerOn(qos_data.at(core_ids_[i]), &sync_points_val);
    if (error != kErrorNone && error != kErrorDeferred) {
      return error;
    }
    sync_points_list.push_back(sync_points_val);
  }

  if (!sync_points_list.size()) {
    return kErrorUndefined;
  }

  // Null Check for the merge
  sync_points_val = sync_points_list[0];
  for (int i = 1; i < sync_points_list.size(); i++) {
    auto val = sync_points_list[i];
    sync_points_val.retire_fence = Fence::Merge(sync_points_val.retire_fence, val.retire_fence);
    sync_points_val.release_fence = Fence::Merge(sync_points_val.release_fence, val.release_fence);
  }

  *sync_points = sync_points_val;
  return error;
}

DisplayError DPUMultiCore::PowerOff(bool teardown, SyncPoints *sync_points) {
  std::vector<SyncPoints> sync_points_list;
  SyncPoints sync_points_val;
  DisplayError error = kErrorNone;

  for (uint32_t i : op_sync_sequence_) {
    error = hw_intf_.at(core_ids_[i])->PowerOff(teardown, &sync_points_val);
    if (error != kErrorNone && error != kErrorDeferred) {
      return error;
    }
    sync_points_list.push_back(sync_points_val);
  }

  if (!sync_points_list.size()) {
    return kErrorUndefined;
  }

  sync_points_val = sync_points_list[0];
  for (int i = 1; i < sync_points_list.size(); i++) {
    auto val = sync_points_list[i];
    sync_points_val.retire_fence = Fence::Merge(sync_points_val.retire_fence, val.retire_fence);
    sync_points_val.release_fence = Fence::Merge(sync_points_val.release_fence, val.release_fence);
  }

  *sync_points = sync_points_val;
  return error;
}

DisplayError DPUMultiCore::Doze(std::map<uint32_t, HWQosData> &qos_data, SyncPoints *sync_points) {
  std::vector<SyncPoints> sync_points_list;
  SyncPoints sync_points_val;
  DisplayError error = kErrorNone;

  for (uint32_t i : op_sync_sequence_) {
    error = hw_intf_.at(core_ids_[i])->Doze(qos_data.at(core_ids_[i]), &sync_points_val);
    if (error != kErrorNone && error != kErrorDeferred) {
      return error;
    }
    sync_points_list.push_back(sync_points_val);
  }

  sync_points_val = sync_points_list[0];
  for (int i = 1; i < sync_points_list.size(); i++) {
    auto val = sync_points_list[i];
    sync_points_val.retire_fence = Fence::Merge(sync_points_val.retire_fence, val.retire_fence);
    sync_points_val.release_fence = Fence::Merge(sync_points_val.release_fence, val.release_fence);
  }

  *sync_points = sync_points_val;
  return error;
}

DisplayError DPUMultiCore::DozeSuspend(std::map<uint32_t, HWQosData> &qos_data,
                                       SyncPoints *sync_points) {
  std::vector<SyncPoints> sync_points_list;
  SyncPoints sync_points_val;
  DisplayError error = kErrorNone;

  for (uint32_t i : op_sync_sequence_) {
    error = hw_intf_.at(core_ids_[i])->DozeSuspend(qos_data.at(core_ids_[i]), &sync_points_val);
    if (error != kErrorNone && error != kErrorDeferred) {
      return error;
    }
    sync_points_list.push_back(sync_points_val);
  }

  sync_points_val = sync_points_list[0];
  for (int i = 1; i < sync_points_list.size(); i++) {
    auto val = sync_points_list[i];
    sync_points_val.retire_fence = Fence::Merge(sync_points_val.retire_fence, val.retire_fence);
    sync_points_val.release_fence = Fence::Merge(sync_points_val.release_fence, val.release_fence);
  }

  *sync_points = sync_points_val;
  return error;
}

DisplayError DPUMultiCore::Standby(SyncPoints *sync_points) {
  std::vector<SyncPoints> sync_points_list;
  SyncPoints sync_points_val;
  DisplayError error = kErrorNone;

  for (auto hw_intf : hw_intf_) {
    error = hw_intf.second->Standby(&sync_points_val);
    if (error != kErrorNone && error != kErrorDeferred) {
      return error;
    }
    sync_points_list.push_back(sync_points_val);
  }

  sync_points_val = sync_points_list[0];
  for (int i = 1; i < sync_points_list.size(); i++) {
    auto val = sync_points_list[i];
    sync_points_val.retire_fence = Fence::Merge(sync_points_val.retire_fence, val.retire_fence);
    sync_points_val.release_fence = Fence::Merge(sync_points_val.release_fence, val.release_fence);
  }

  *sync_points = sync_points_val;
  return error;
}

DisplayError DPUMultiCore::Validate(std::map<uint32_t, HWLayersInfo> &hw_layers_info) {
  for (auto hw_intf : hw_intf_) {
    DisplayError error = hw_intf.second->Validate(&hw_layers_info.at(hw_intf.first));
    if (error != kErrorNone) {
      return error;
    }
  }

  return kErrorNone;
}

DisplayError DPUMultiCore::Commit(std::map<uint32_t, HWLayersInfo> &hw_layers_info) {
  for (uint32_t i : op_sync_sequence_) {
    DisplayError error = hw_intf_.at(core_ids_[i])->Commit(&hw_layers_info.at(core_ids_[i]));
    if (error != kErrorNone) {
      return error;
    }
  }

  shared_ptr<Fence> retire_fence = hw_layers_info.at(core_ids_[0]).retire_fence;
  shared_ptr<Fence> sync_handle = hw_layers_info.at(core_ids_[0]).sync_handle;
  shared_ptr<Fence> op_release_fence =
      hw_layers_info.at(core_ids_[0]).output_buffer
          ? hw_layers_info.at(core_ids_[0]).output_buffer->release_fence
          : nullptr;

#ifndef SDM_VIRTUAL_DRIVER
  if (!retire_fence || !sync_handle) {
    return kErrorUndefined;
  }
#endif

  for (auto &layers_info : hw_layers_info) {
    retire_fence = Fence::Merge(layers_info.second.retire_fence, retire_fence);
    sync_handle = Fence::Merge(layers_info.second.sync_handle, sync_handle);

    if (layers_info.second.output_buffer) {
      op_release_fence =
          Fence::Merge(layers_info.second.output_buffer->release_fence, op_release_fence);
    }
  }

  for (auto &layers_info : hw_layers_info) {
    if (!layers_info.second.output_buffer) {
      continue;
    }

    layers_info.second.output_buffer->release_fence = op_release_fence;
  }

  hw_layers_info.at(core_ids_[0]).common_info->retire_fence = retire_fence;
  hw_layers_info.at(core_ids_[0]).common_info->sync_handle = sync_handle;

  for (auto &layers_info : hw_layers_info) {
    for (auto &layer : layers_info.second.hw_layers) {
      layer.input_buffer.release_fence = sync_handle;
    }
  }

  // To-Do: Revisit update_mask
  hw_layers_info.at(core_ids_[0]).common_info->updates_mask = 0;
  return kErrorNone;
}

DisplayError DPUMultiCore::Flush(std::map<uint32_t, HWLayersInfo> &hw_layers_info) {
  for (auto hw_intf : hw_intf_) {
    DisplayError error = hw_intf.second->Flush(&hw_layers_info.at(hw_intf.first));
    if (error != kErrorNone) {
      return error;
    }
  }

  return kErrorNone;
}

DisplayError DPUMultiCore::GetPPFeaturesVersion(PPFeatureVersion *vers, uint32_t core_id) {
  return hw_intf_.at(core_id)->GetPPFeaturesVersion(vers);
}

DisplayError DPUMultiCore::SetPPFeature(PPFeatureInfo *feature, uint32_t &core_id) {
  DisplayError error = kErrorNone;

  if (!feature) {
    DLOGE("Invalid null feature!");
    return kErrorParameters;
  }

  if (hw_intf_.find(core_id) == hw_intf_.end()) {
    DLOGE("hw_intf is not present for core_id=%u", core_id);
    return kErrorNotSupported;
  }

  error = hw_intf_[core_id]->SetPPFeature(feature);
  if (error != kErrorNone) {
    DLOGE("Failed to set pp feature for core_id=%u", core_id);
    return error;
  }

  return error;
}

DisplayError DPUMultiCore::SetVSyncState(bool enable) {
  for (auto hw_intf : hw_intf_) {
    DisplayError error = hw_intf.second->SetVSyncState(enable);
    if (error != kErrorNone) {
      return error;
    }
  }
  return kErrorNone;
}

void DPUMultiCore::SetIdleTimeoutMs(uint32_t timeout_ms) {
  for (auto hw_intf : hw_intf_) {
    hw_intf.second->SetIdleTimeoutMs(timeout_ms);
  }
}

DisplayError DPUMultiCore::SetDisplayMode(const HWDisplayMode hw_display_mode) {
  for (auto hw_intf : hw_intf_) {
    DisplayError error = hw_intf.second->SetDisplayMode(hw_display_mode);
    if (error != kErrorNone) {
      return error;
    }
  }

  return kErrorNone;
}

DisplayError DPUMultiCore::SetRefreshRate(uint32_t refresh_rate) {
  for (auto hw_intf : hw_intf_) {
    DisplayError error = hw_intf.second->SetRefreshRate(refresh_rate);
    if (error != kErrorNone) {
      return error;
    }
  }

  return kErrorNone;
}

DisplayError DPUMultiCore::SetPanelBrightness(int level) {
  for (auto hw_intf : hw_intf_) {
    DisplayError error = hw_intf.second->SetPanelBrightness(level);
    if (error != kErrorNone) {
      return error;
    }
  }

  return kErrorNone;
}

DisplayError DPUMultiCore::GetHWScanInfo(HWScanInfo *scan_info) {
  std::vector<HWScanInfo> scan_info_list;
  HWScanInfo scan_info_val;

  for (auto hw_intf : hw_intf_) {
    DisplayError error = hw_intf.second->GetHWScanInfo(&scan_info_val);
    if (error != kErrorNone) {
      return error;
    }

    scan_info_list.push_back(scan_info_val);
  }

  if (!AreAllEntriesSame<HWScanInfo>(scan_info_list)) {
    return kErrorUndefined;
  }
  *scan_info = scan_info_list[0];

  return kErrorNone;
}

DisplayError DPUMultiCore::GetVideoFormat(uint32_t config_index, uint32_t *video_format) {
  std::vector<uint32_t> video_format_list;
  uint32_t video_format_val = 0;

  for (auto hw_intf : hw_intf_) {
    DisplayError error = hw_intf.second->GetVideoFormat(config_index, &video_format_val);
    if (error != kErrorNone) {
      return error;
    }

    video_format_list.push_back(video_format_val);
  }

  if (!AreAllEntriesSame<uint32_t>(video_format_list)) {
    return kErrorUndefined;
  }
  *video_format = video_format_list[0];

  return kErrorNone;
}

DisplayError DPUMultiCore::GetMaxCEAFormat(uint32_t *max_cea_format) {
  std::vector<uint32_t> max_cea_format_list;
  uint32_t max_cea_format_val = 0;

  for (auto hw_intf : hw_intf_) {
    DisplayError error = hw_intf.second->GetMaxCEAFormat(&max_cea_format_val);
    if (error != kErrorNone) {
      return error;
    }

    max_cea_format_list.push_back(max_cea_format_val);
  }

  if (!AreAllEntriesSame<uint32_t>(max_cea_format_list)) {
    return kErrorUndefined;
  }
  *max_cea_format = max_cea_format_list[0];

  return kErrorNone;
}

DisplayError DPUMultiCore::SetCursorPosition(std::map<uint32_t, HWLayersInfo> &hw_layers_info,
                                             int x, int y) {
  for (auto hw_intf : hw_intf_) {
    DisplayError error = hw_intf.second->SetCursorPosition(&hw_layers_info.at(hw_intf.first), x, y);
    if (error != kErrorNone) {
      return error;
    }
  }

  return kErrorNone;
}

DisplayError DPUMultiCore::OnMinHdcpEncryptionLevelChange(uint32_t min_enc_level) {
  for (auto hw_intf : hw_intf_) {
    DisplayError error = hw_intf.second->OnMinHdcpEncryptionLevelChange(min_enc_level);
    if (error != kErrorNone) {
      return error;
    }
  }

  return kErrorNone;
}

DisplayError DPUMultiCore::GetPanelBrightness(int *level) {
  std::vector<int> level_list;
  int level_val = 0;

  for (auto hw_intf : hw_intf_) {
    DisplayError error = hw_intf.second->GetPanelBrightness(&level_val);
    if (error != kErrorNone) {
      return error;
    }

    level_list.push_back(level_val);
  }

  if (!AreAllEntriesSame<int>(level_list)) {
    return kErrorUndefined;
  }
  *level = level_list[0];

  return kErrorNone;
}

DisplayError DPUMultiCore::SetAutoRefresh(bool enable) {
  for (auto hw_intf : hw_intf_) {
    DisplayError error = hw_intf.second->SetAutoRefresh(enable);
    if (error != kErrorNone) {
      return error;
    }
  }

  return kErrorNone;
}

DisplayError DPUMultiCore::SetScaleLutConfig(HWScaleLutInfo *lut_info) {
  for (auto hw_intf : hw_intf_) {
    DisplayError error = hw_intf.second->SetScaleLutConfig(lut_info);
    if (error != kErrorNone) {
      return error;
    }
  }
  return kErrorNone;
}

DisplayError DPUMultiCore::UnsetScaleLutConfig() {
  for (auto hw_intf : hw_intf_) {
    DisplayError error = hw_intf.second->UnsetScaleLutConfig();
    if (error != kErrorNone) {
      return error;
    }
  }

  return kErrorNone;
}

DisplayError DPUMultiCore::SetMixerAttributes(const HWMixerAttributes &mixer_attributes) {
  for (auto hw_intf : hw_intf_) {
    HWMixerAttributes dpu_mixer = mixer_attributes;
    dpu_mixer.width = dpu_mixer.width / hw_intf_.size();

    DisplayError error = hw_intf.second->SetMixerAttributes(dpu_mixer);
    if (error != kErrorNone) {
      return error;
    }
  }

  return kErrorNone;
}

DisplayError DPUMultiCore::GetMixerAttributes(DisplayDeviceContext *device_ctx,
                                              DisplayClientContext *client_ctx) {
  DisplayError error = kErrorNone;
  std::map<uint32_t, HWMixerAttributes> mixer_attr_map;

  for (auto hw_intf : hw_intf_) {
    error = hw_intf.second->GetMixerAttributes(&mixer_attr_map[hw_intf.first]);
    if (error != kErrorNone) {
      return error;
    }
  }

  client_ctx->mixer_attributes = mixer_attr_map[core_ids_[0]];
  DisplayInfoContext info_ctx = {};
  info_ctx.mixer_attributes = mixer_attr_map[core_ids_[0]];
  if (device_ctx->find(core_ids_[0]) == device_ctx->end()) {
    device_ctx->insert({core_ids_[0], info_ctx});
  } else {
    device_ctx->at(core_ids_[0]).mixer_attributes = info_ctx.mixer_attributes;
  }

  auto i = mixer_attr_map.begin();
  i++;
  for (; i != mixer_attr_map.end(); i++) {
    auto itr = device_ctx->find(i->first);
    if (itr == device_ctx->end()) {
      DisplayInfoContext info_ctx = {};
      info_ctx.mixer_attributes = i->second;
      device_ctx->insert({i->first, info_ctx});
    } else {
      itr->second.mixer_attributes = i->second;
    }
    device_ctx->at(i->first).mixer_attributes = i->second;
    client_ctx->mixer_attributes.width += i->second.width;
    client_ctx->mixer_attributes.split_left += i->second.split_left;
  }
  return kErrorNone;
}

DisplayError DPUMultiCore::DumpDebugData() {
  for (auto hw_intf : hw_intf_) {
    DisplayError error = hw_intf.second->DumpDebugData();
    if (error != kErrorNone) {
      return error;
    }
  }

  return kErrorNone;
}

DisplayError DPUMultiCore::SetDppsFeature(void *payload, size_t size) {
  for (auto hw_intf : hw_intf_) {
    DisplayError error = hw_intf.second->SetDppsFeature(payload, size);
    if (error != kErrorNone) {
      return error;
    }
  }

  return kErrorNone;
}

DisplayError DPUMultiCore::SetPPConfig(void *payload, size_t size) {
  for (auto hw_intf : hw_intf_) {
    DisplayError error = hw_intf.second->SetPPConfig(payload, size);
    if (error != kErrorNone) {
      return error;
    }
  }

  return kErrorNone;
}

DisplayError DPUMultiCore::GetDppsFeatureInfo(void *payload, size_t size) {
  return hw_intf_.at(core_ids_[0])->GetDppsFeatureInfo(payload, size);
}

DisplayError DPUMultiCore::HandleSecureEvent(SecureEvent secure_event,
                                             std::map<uint32_t, HWQosData> &qos_data) {
  for (int i = 0; i < hw_intf_.size(); i++) {
    DisplayError error =
        hw_intf_.at(core_ids_[i])->HandleSecureEvent(secure_event, qos_data.at(core_ids_[i]));
    if (error != kErrorNone) {
      return error;
    }
  }

  return kErrorNone;
}

DisplayError DPUMultiCore::ControlIdlePowerCollapse(bool enable, bool synchronous) {
  for (auto hw_intf : hw_intf_) {
    DisplayError error = hw_intf.second->ControlIdlePowerCollapse(enable, synchronous);
    if (error != kErrorNone) {
      return error;
    }
  }

  return kErrorNone;
}

DisplayError DPUMultiCore::SetDisplayDppsAdROI(void *payload) {
  for (auto hw_intf : hw_intf_) {
    DisplayError error = hw_intf.second->SetDisplayDppsAdROI(payload);
    if (error != kErrorNone) {
      return error;
    }
  }

  return kErrorNone;
}

DisplayError DPUMultiCore::SetDynamicDSIClock(uint64_t bit_clk_rate) {
  for (auto hw_intf : hw_intf_) {
    DisplayError error = hw_intf.second->SetDynamicDSIClock(bit_clk_rate);
    if (error != kErrorNone) {
      return error;
    }
  }

  return kErrorNone;
}

DisplayError DPUMultiCore::GetDynamicDSIClock(uint64_t *bit_clk_rate) {
  std::vector<uint64_t> bit_clk_rate_list;
  uint64_t bit_clk_rate_val = 0;

  for (auto hw_intf : hw_intf_) {
    DisplayError error = hw_intf.second->GetDynamicDSIClock(&bit_clk_rate_val);
    if (error != kErrorNone) {
      return error;
    }

    bit_clk_rate_list.push_back(bit_clk_rate_val);
  }

  if (!AreAllEntriesSame<uint64_t>(bit_clk_rate_list)) {
    return kErrorUndefined;
  }
  *bit_clk_rate = bit_clk_rate_list[0];

  return kErrorNone;
}

DisplayError DPUMultiCore::GetDisplayIdentificationData(uint8_t *out_port, uint32_t *out_data_size,
                                                        uint8_t *out_data) {
  uint8_t out_port_temp = 0;
  *out_port = 0;
  for (auto hw_intf : hw_intf_) {
    DisplayError error =
        hw_intf.second->GetDisplayIdentificationData(&out_port_temp, out_data_size, out_data);
    if (error != kErrorNone) {
      return error;
    }
    *out_port = *out_port | out_port_temp;
  }
  return kErrorNone;
}

DisplayError DPUMultiCore::SetFrameTrigger(FrameTriggerMode mode) {
  for (auto hw_intf : hw_intf_) {
    DisplayError error = hw_intf.second->SetFrameTrigger(mode);
    if (error != kErrorNone) {
      return error;
    }
  }

  return kErrorNone;
}

DisplayError DPUMultiCore::SetFrameTrigger(FrameTriggerMode mode, uint32_t core_id) {
  DisplayError error = hw_intf_.at(core_id)->SetFrameTrigger(mode);
  return error;
}

DisplayError DPUMultiCore::SetBLScale(uint32_t level) {
  for (auto hw_intf : hw_intf_) {
    DisplayError error = hw_intf.second->SetBLScale(level);
    if (error != kErrorNone) {
      return error;
    }
  }

  return kErrorNone;
}

DisplayError DPUMultiCore::GetPanelBlMaxLvl(uint32_t *max_bl) {
  std::vector<uint32_t> max_bl_list;
  uint32_t max_bl_val = 0;

  for (auto hw_intf : hw_intf_) {
    DisplayError error = hw_intf.second->GetPanelBlMaxLvl(&max_bl_val);
    if (error != kErrorNone) {
      return error;
    }

    max_bl_list.push_back(max_bl_val);
  }

  if (!AreAllEntriesSame<uint32_t>(max_bl_list)) {
    return kErrorUndefined;
  }
  *max_bl = max_bl_list[0];

  return kErrorNone;
}

DisplayError DPUMultiCore::GetPanelBrightnessBasePath(std::string *base_path) const {
  return hw_intf_.at(core_ids_[0])->GetPanelBrightnessBasePath(base_path);
}

DisplayError DPUMultiCore::SetBlendSpace(const PrimariesTransfer &blend_space) {
  for (auto hw_intf : hw_intf_) {
    DisplayError error = hw_intf.second->SetBlendSpace(blend_space);
    if (error != kErrorNone) {
      return error;
    }
  }

  return kErrorNone;
}

DisplayError DPUMultiCore::EnableSelfRefresh(SelfRefreshState self_refresh_state) {
  for (auto hw_intf : hw_intf_) {
    DisplayError error = hw_intf.second->EnableSelfRefresh(self_refresh_state);
    if (error != kErrorNone) {
      return error;
    }
  }

  return kErrorNone;
}

DisplayError DPUMultiCore::GetFeatureSupportStatus(const HWFeature feature, uint32_t *status) {
  std::vector<uint32_t> status_list;
  uint32_t status_val;

  for (auto hw_intf : hw_intf_) {
    DisplayError error = hw_intf.second->GetFeatureSupportStatus(feature, &status_val);
    if (error != kErrorNone) {
      return error;
    }

    status_list.push_back(status_val);
  }

  if (!AreAllEntriesSame<uint32_t>(status_list)) {
    return kErrorUndefined;
  }
  *status = status_list[zero_index];

  return kErrorNone;
}

void DPUMultiCore::FlushConcurrentWriteback() {
  for (auto hw_intf : hw_intf_) {
    hw_intf.second->FlushConcurrentWriteback();
  }
}

DisplayError DPUMultiCore::SetAlternateDisplayConfig(uint32_t *alt_config) {
  for (auto hw_intf : hw_intf_) {
    DisplayError error = hw_intf.second->SetAlternateDisplayConfig(alt_config);
    if (error != kErrorNone) {
      return error;
    }
  }

  return kErrorNone;
}

DisplayError DPUMultiCore::GetQsyncFps(uint32_t *qsync_fps) {
  std::vector<uint32_t> qsync_fps_list;
  uint32_t qsync_fps_val;

  for (auto hw_intf : hw_intf_) {
    DisplayError error = hw_intf.second->GetQsyncFps(&qsync_fps_val);
    if (error != kErrorNone) {
      return error;
    }

    qsync_fps_list.push_back(qsync_fps_val);
  }

  if (!AreAllEntriesSame<uint32_t>(qsync_fps_list)) {
    return kErrorUndefined;
  }
  *qsync_fps = qsync_fps_list[zero_index];

  return kErrorNone;
}

DisplayError DPUMultiCore::CancelDeferredPowerMode() {
  for (auto hw_intf : hw_intf_) {
    DisplayError error = hw_intf.second->CancelDeferredPowerMode();
    if (error != kErrorNone) {
      return error;
    }
  }

  return kErrorNone;
}

PanelFeaturePropertyIntf *DPUMultiCore::GetPanelFeaturePropertyIntf() {
  return hw_intf_.at(core_ids_[0])->GetPanelFeaturePropertyIntf();
}

void DPUMultiCore::GetHWInterface(HWInterface **intf) {
  *intf = hw_intf_.at(core_ids_[0]);
}

template <typename T>
bool DPUMultiCore::AreAllEntriesSame(std::vector<T> &entries) {
  if (!entries.size()) {
    return true;
  }

  T val = entries[0];
  for (int i = 1; i < entries.size(); i++) {
    if (val != entries[i]) {
      return false;
    }
  }

  return true;
}

void DPUMultiCore::GetDRMDisplayToken(sde_drm::DRMDisplayToken *token) const {
  hw_intf_.at(core_ids_[0])->GetDRMDisplayToken(token);
}

DisplayError DPUMultiCore::GetFbConfig(uint32_t width, uint32_t height,
                                       DisplayDeviceContext *device_ctx,
                                       DisplayClientContext *client_ctx) {
  DisplayError error = kErrorNone;
  client_ctx->fb_config.x_pixels = width;
  client_ctx->fb_config.y_pixels = height;

  for (uint32_t i = 0; i < device_ctx->size(); i++) {
    device_ctx->at(core_ids_[i]).fb_config = client_ctx->fb_config;

    uint32_t dpu_fb_width =
        client_ctx->fb_config.x_pixels * (device_ctx->at(core_ids_[i]).display_attributes.x_pixels /
                                          client_ctx->display_attributes.x_pixels);
    device_ctx->at(core_ids_[i]).fb_config.x_pixels = dpu_fb_width;
  }
  return error;
}

}  // namespace sdm
