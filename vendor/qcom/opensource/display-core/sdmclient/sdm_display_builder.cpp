/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include <utils/debug.h>

#include <algorithm>
#include <map>
#include <vector>

#include "sdm_display_builder.h"

#define __CLASS__ "SDMDisplayBuilder"

namespace sdm {

Locker SDMDisplayBuilder::hdr_locker_[kNumDisplays];

std::map<Display, DisplayMapInfo *> &SDMDisplayBuilder::GetActiveDisplays() {
  return map_active_displays_;
}

int SDMDisplayBuilder::GetDisplayIndex(int dpy) {
  DisplayMapInfo *map_info = nullptr;
  switch (dpy) {
    case qdutilsDisplayType::DISPLAY_PRIMARY:
      map_info = &map_info_primary_[0];
      break;
    case qdutilsDisplayType::DISPLAY_EXTERNAL:
      map_info = map_info_pluggable_.size() ? &map_info_pluggable_[0] : nullptr;
      break;
    case qdutilsDisplayType::DISPLAY_EXTERNAL_2:
      map_info = (map_info_pluggable_.size() > 1) ? &map_info_pluggable_[1] : nullptr;
      break;
    case qdutilsDisplayType::DISPLAY_VIRTUAL:
      map_info = map_info_virtual_.size() ? &map_info_virtual_[0] : nullptr;
      break;
    case qdutilsDisplayType::DISPLAY_VIRTUAL_2:
      map_info = (map_info_virtual_.size() > 1) ? &map_info_virtual_[1] : nullptr;
      break;
    case qdutilsDisplayType::DISPLAY_BUILTIN_2:
      map_info = map_info_builtin_.size() ? &map_info_builtin_[0] : nullptr;
      break;
    default:
      DLOGW("Unknown display %d.", dpy);
      break;
  }

  if (!map_info) {
    DLOGW("Display index not found for display %d.", dpy);
    return -1;
  }

  return INT(map_info->client_id);
}

bool SDMDisplayBuilder::IsHDRDisplay(uint32_t client_id) {
  SCOPE_LOCK(hdr_locker_[client_id]);

  if (is_hdr_display_.size() <= client_id) {
    return false;
  }

  return is_hdr_display_[client_id];
}

std::vector<DisplayMapInfo> &
SDMDisplayBuilder::GetDisplayMapInfo(int display_id) {
  switch (display_id) {
    case qdutilsDisplayType::DISPLAY_PRIMARY:
      return map_info_primary_;

    case qdutilsDisplayType::DISPLAY_EXTERNAL:
    case qdutilsDisplayType::DISPLAY_EXTERNAL_2:
      return map_info_pluggable_;

    case qdutilsDisplayType::DISPLAY_VIRTUAL:
    case qdutilsDisplayType::DISPLAY_VIRTUAL_2:
      return map_info_virtual_;

    case qdutilsDisplayType::DISPLAY_BUILTIN_2:
      return map_info_builtin_;

    default:
      DLOGW("Unknown display %d", display_id);
  }

  return map_info_primary_;
}

bool SDMDisplayBuilder::HasHDRSupport(SDMDisplay *sdm_display) {
  // query number of hdr types
  uint32_t out_num_types = 0;
  float out_max_luminance = 0.0f;
  float out_max_average_luminance = 0.0f;
  float out_min_luminance = 0.0f;
  if (sdm_display->GetHdrCapabilities(
          &out_num_types, nullptr, &out_max_luminance,
          &out_max_average_luminance, &out_min_luminance) != kErrorNone) {
    return false;
  }

  return (out_num_types > 0);
}

void SDMDisplayBuilder::Init(Locker *locker) {
  // Default slots:
  //    Primary = 0, External = 1
  //    Additional external displays 2,3,...max_pluggable_count.
  //    Additional builtin displays max_pluggable_count + 1, max_pluggable_count
  //    + 2,... Last slots for virtual displays.
  // Virtual display id is only for SF <--> SDM communication.
  // It need not align with hwccomposer_defs

  locker_ = locker;

  SDMDebugHandler::Get()->GetProperty(DISABLE_HOTPLUG_BWCHECK,
                                      &disable_hotplug_bwcheck_);
  DLOGI("disable_hotplug_bwcheck_: %d", disable_hotplug_bwcheck_);

  SDMDebugHandler::Get()->GetProperty(ENABLE_PRIMARY_RECONFIG_REQUEST,
                                      &enable_primary_reconfig_req_);
  DLOGI("enable_primary_reconfig_req_: %d", enable_primary_reconfig_req_);

  DisplayMapInfo primary_info{};
  primary_info.client_id = qdutilsDisplayType::DISPLAY_PRIMARY;
  map_info_primary_.push_back(primary_info);

  HWDisplayInterfaceInfo hw_disp_info = {};
  DisplayError error = core_intf_->GetFirstDisplayInterfaceType(&hw_disp_info);
  if (error != kErrorNone) {
    CoreInterface::DestroyCore();
    DLOGE("Primary display type not recognized. Error = %d", error);
    return;
  }

  int max_builtin = 0;
  int max_pluggable = 0;
  int max_virtual = 0;

  error = core_intf_->GetMaxDisplaysSupported(kBuiltIn, &max_builtin);
  if (error != kErrorNone) {
    CoreInterface::DestroyCore();
    DLOGE("Could not find maximum built-in displays supported. Error = %d",
          error);
    return;
  }

  error = core_intf_->GetMaxDisplaysSupported(kPluggable, &max_pluggable);
  if (error != kErrorNone) {
    CoreInterface::DestroyCore();
    DLOGE("Could not find maximum pluggable displays supported. Error = %d",
          error);
    return;
  }

  error = core_intf_->GetMaxDisplaysSupported(kVirtual, &max_virtual);
  if (error != kErrorNone) {
    CoreInterface::DestroyCore();
    DLOGE("Could not find maximum virtual displays supported. Error = %d",
          error);
    return;
  }

  if (max_virtual == 0) {
    // Check if WB using GPU is supported.
    max_virtual +=
        virtual_display_factory_.IsGPUColorConvertSupported() ? 1 : 0;
  }

  if (kPluggable == hw_disp_info.type) {
    // If primary is a pluggable display, we have already used one pluggable
    // display interface.
    // max_pluggable/builtin can both be initialized to 0 in case of invalid panel node
    // check to avoid overflow
    if (max_pluggable) {
      max_pluggable--;
    }
  } else {
    if (max_builtin) {
      max_builtin--;
    }
  }

  // Init slots in accordance to h/w capability.
  uint32_t disp_count = UINT32(std::min(max_pluggable, kNumPluggable));
  Display base_id = qdutilsDisplayType::DISPLAY_EXTERNAL;
  map_info_pluggable_.resize(disp_count);
  for (auto &map_info : map_info_pluggable_) {
    map_info.client_id = base_id++;
  }

  disp_count = UINT32(std::min(max_builtin, kNumBuiltIn));
  map_info_builtin_.resize(disp_count);
  for (auto &map_info : map_info_builtin_) {
    map_info.client_id = base_id++;
  }

  disp_count = UINT32(std::min(max_virtual, kNumVirtual));
  map_info_virtual_.resize(disp_count);
  for (auto &map_info : map_info_virtual_) {
    map_info.client_id = base_id++;
  }

  // resize HDR supported map to total number of displays.
  is_hdr_display_.resize(UINT32(base_id));

  GetVirtualDisplayList();
}

Display SDMDisplayBuilder::GetActiveBuiltinDisplay() {
  Display active_display = kNumDisplays;
  // Get first active display among primary and built-in displays.
  std::vector<DisplayMapInfo> map_info = map_info_primary_;
  std::copy(map_info_builtin_.begin(), map_info_builtin_.end(),
            std::back_inserter(map_info));

  for (auto &info : map_info) {
    Display target_display = info.client_id;
    auto sdm_display = cb_->GetDisplayFromClientId(target_display);
    if (sdm_display &&
        sdm_display->GetCurrentPowerMode() != SDMPowerMode::POWER_MODE_OFF) {
      active_display = info.client_id;
      break;
    }
  }

  return active_display;
}

void SDMDisplayBuilder::Deinit() {
  // Destroy all connected displays
  DestroyDisplay(&map_info_primary_[0]);

  for (auto &map_info : map_info_builtin_) {
    DestroyDisplay(&map_info);
  }

  for (auto &map_info : map_info_pluggable_) {
    DestroyDisplay(&map_info);
  }

  for (auto &map_info : map_info_virtual_) {
    DestroyDisplay(&map_info);
  }

  DisplayError error = CoreInterface::DestroyCore();
  if (error != kErrorNone) {
    DLOGE("Display core de-initialization failed. Error = %d", error);
  }
}

DisplayError SDMDisplayBuilder::CreateVirtualDisplay(uint32_t width,
                                                     uint32_t height,
                                                     int32_t *format,
                                                     Display *out_display_id) {
  if (!out_display_id || !width || !height || !format) {
    return kErrorParameters;
  }

  auto status = CreateVirtualDisplayObj(width, height, format, out_display_id);
  if (status == kErrorNone) {
    DLOGI("Created virtual display id:%" PRIu64 ", res: %dx%d", *out_display_id,
          width, height);
  } else {
    DLOGW("Failed to create virtual display: %s", to_string(status).c_str());
  }
  return status;
}

DisplayError SDMDisplayBuilder::DestroyVirtualDisplay(Display display) {
  if (display >= kNumDisplays) {
    return kErrorParameters;
  }

  for (auto &map_info : map_info_virtual_) {
    if (map_info.client_id == display) {
      DLOGI("Destroying virtual display id:%" PRIu64, display);
      DestroyDisplay(&map_info);
      break;
    }
  }

  auto it = virtual_id_map_.find(display);
  if (it != virtual_id_map_.end()) {
    virtual_id_map_.erase(it);
  }

  return kErrorNone;
}

DisplayError SDMDisplayBuilder::CreateVirtualDisplayObj(
    uint32_t width, uint32_t height, int32_t *format, Display *out_display_id) {
  // Get virtual display from cache if already created
  for (auto &vds_map : virtual_id_map_) {
    if (vds_map.second.width == width && vds_map.second.height == height &&
        vds_map.second.format == *format && !vds_map.second.in_use) {
      vds_map.second.in_use = true;
      *out_display_id = vds_map.first;
      return kErrorNone;
    }
  }

  Display active_builtin_disp_id = GetActiveBuiltinDisplay();
  Display client_id = kNumDisplays;
  if (active_builtin_disp_id < kNumDisplays) {
    SEQUENCE_WAIT_SCOPE_LOCK(locker_[active_builtin_disp_id]);
    std::bitset<kSecureMax> secure_sessions = 0;
    auto disp = cb_->GetDisplayFromClientId(active_builtin_disp_id);
    if (disp) {
      disp->GetActiveSecureSession(&secure_sessions);
    }
    if (secure_sessions.any()) {
      DLOGW("Secure session is active, cannot create virtual display.");
      return kErrorNotSupported;
    } else if (IsVirtualDisplayConnected()) {
      DLOGW(
          "Previous virtual session is active, cannot create virtual display.");
      return kErrorNotSupported;
    } else if (IsPluggableDisplayConnected()) {
      DLOGW("External session is active, cannot create virtual display.");
      return kErrorNotSupported;
    }
  }

  // Request to get virtual display id corresponds writeback block, which could
  // be used for WFD.
  int32_t display_id = -1;
  auto err = core_intf_->RequestVirtualDisplayId(&display_id);
  if (err != kErrorNone || display_id == -1) {
    return kErrorResources;
  }

  // Lock confined to this scope
  for (auto &map_info : map_info_virtual_) {
    client_id = map_info.client_id;
    {
      SCOPE_LOCK(locker_[client_id]);
      auto sdm_display = cb_->GetDisplayFromClientId(client_id);
      if (sdm_display) {
        continue;
      }

      int status = -EINVAL;
      status = virtual_display_factory_.Create(
          core_intf_, buffer_allocator_, callbacks_, client_id, display_id,
          width, height, format, set_min_lum_, set_max_lum_, &sdm_display);
      if (display_id == -1 || status) {
        return kErrorResources;
      }

      {
        SCOPE_LOCK(hdr_locker_[client_id]);
        is_hdr_display_[UINT32(client_id)] = HasHDRSupport(sdm_display);
      }

      cb_->SetDisplayByClientId(client_id, sdm_display);

      DLOGI("Created virtual display client id:%" PRIu64
            ", display_id: %d with res: %dx%d",
            client_id, display_id, width, height);

      *out_display_id = client_id;
      map_info.disp_type = kVirtual;
      map_info.sdm_id = display_id;
      map_active_displays_.insert(std::make_pair(client_id, &map_info));

      VirtualDisplayData vds_data;
      vds_data.width = width;
      vds_data.height = height;
      vds_data.format = *format;
      virtual_id_map_.insert(std::make_pair(client_id, vds_data));

      return kErrorNone;
    }
  }

  return kErrorResources;
}

bool SDMDisplayBuilder::IsPluggableDisplayConnected() {
  for (auto &map_info : map_info_pluggable_) {
    if (cb_->GetDisplayFromClientId(map_info.client_id)) {
      return true;
    }
  }
  return false;
}

bool SDMDisplayBuilder::IsVirtualDisplayConnected() {
  bool connected = true;

  for (auto &map_info : map_info_virtual_) {
    connected &= !!cb_->GetDisplayFromClientId(map_info.client_id);
  }

  return connected;
}

void SDMDisplayBuilder::GetVirtualDisplayList() {
  HWDisplaysInfo hw_displays_info = {};
  core_intf_->GetDisplaysStatus(&hw_displays_info);

  for (auto &iter : hw_displays_info) {
    auto &info = iter.second;
    if (info.display_type != kVirtual) {
      continue;
    }

    virtual_display_list_.push_back(info);
  }
}

uint32_t SDMDisplayBuilder::GetVirtualDisplayCount() {
  return virtual_display_list_.size();
}

int SDMDisplayBuilder::CreatePrimaryDisplay() {
  int status = -EINVAL;
  HWDisplaysInfo hw_displays_info = {};

  DisplayError error = core_intf_->GetDisplaysStatus(&hw_displays_info);
  if (error != kErrorNone) {
    DLOGE("Failed to get connected display list. Error = %d", error);
    return status;
  }

  for (auto &iter : hw_displays_info) {
    auto &info = iter.second;
    if (!info.is_primary) {
      continue;
    }

    // todo (user): If primary display is not connected (e.g. hdmi as primary),
    // a NULL display need to be created. SF expects primary display hotplug
    // during callback registration unlike previous implementation where first
    // hotplug could be notified anytime.
    if (!info.is_connected) {
      DLOGE("Primary display is not connected. Not supported at present.");
      break;
    }

    SDMDisplay *sdm_display = nullptr;
    Display client_id = map_info_primary_[0].client_id;

    if (info.display_type == kBuiltIn) {
      status = SDMDisplayBuiltIn::Create(core_intf_, buffer_allocator_,
                                         callbacks_, evt_handler_, client_id,
                                         info.display_id, &sdm_display);
    } else if (info.display_type == kPluggable) {
      status = SDMDisplayPluggable::Create(
          core_intf_, buffer_allocator_, callbacks_, evt_handler_, client_id,
          info.display_id, 0, 0, false, &sdm_display);
    } else {
      DLOGE("Spurious primary display type = %d", info.display_type);
      break;
    }

    if (!status) {
      DLOGI("Created primary display type = %d, sdm id = %d, client id = %d",
            info.display_type, info.display_id, UINT32(client_id));

      {
        SCOPE_LOCK(hdr_locker_[client_id]);
        is_hdr_display_[UINT32(client_id)] = HasHDRSupport(sdm_display);
      }

      map_info_primary_[0].disp_type = info.display_type;
      map_info_primary_[0].sdm_id = info.display_id;

      map_active_displays_.insert(
          std::make_pair(client_id, &map_info_primary_[0]));
      cb_->SetDisplayByClientId(client_id, sdm_display);
    } else {
      DLOGE("Primary display creation has failed! status = %d", status);
      return status;
    }

    // Primary display is found, no need to parse more.
    break;
  }

  return status;
}

int SDMDisplayBuilder::HandleBuiltInDisplays() {
  HWDisplaysInfo hw_displays_info = {};
  DisplayError error = core_intf_->GetDisplaysStatus(&hw_displays_info);
  if (error != kErrorNone) {
    DLOGE("Failed to get connected display list. Error = %d", error);
    return -EINVAL;
  }

  int status = 0;
  for (auto &iter : hw_displays_info) {
    auto &info = iter.second;

    // Do not recreate primary display.
    if (info.is_primary || info.display_type != kBuiltIn) {
      continue;
    }

    for (auto &map_info : map_info_builtin_) {
      Display client_id = map_info.client_id;
      SCOPE_LOCK(locker_[client_id]);

      auto disp = cb_->GetDisplayFromClientId(client_id);
      if (disp) {
        continue;
      }

      DLOGI("Create builtin display, sdm id = %d, client id = %d",
            info.display_id, UINT32(client_id));
      status = SDMDisplayBuiltIn::Create(core_intf_, buffer_allocator_,
                                         callbacks_, evt_handler_, client_id,
                                         info.display_id, &disp);
      if (status) {
        DLOGE("Builtin display creation failed.");
        break;
      }

      {
        SCOPE_LOCK(hdr_locker_[client_id]);
        is_hdr_display_[UINT32(client_id)] = HasHDRSupport(disp);
      }

      DLOGI("Builtin display created: sdm id = %d, client id = %d",
            info.display_id, UINT32(client_id));
      map_info.disp_type = info.display_type;
      map_info.sdm_id = info.display_id;

      map_active_displays_.insert(std::make_pair(client_id, &map_info));
      cb_->SetDisplayByClientId(client_id, disp);

      DLOGI("Hotplugging builtin display, sdm id = %d, client id = %d",
            info.display_id, UINT32(client_id));
      callbacks_->OnHotplug(client_id, true);
      break;
    }
  }

  return status;
}

bool SDMDisplayBuilder::IsHWDisplayConnected(Display client_id) {
  HWDisplaysInfo hw_displays_info = {};

  DisplayError error = core_intf_->GetDisplaysStatus(&hw_displays_info);
  if (error != kErrorNone) {
    DLOGE("Failed to get connected display list. Error = %d", error);
    return false;
  }

  auto itr_map =
      std::find_if(map_info_pluggable_.begin(), map_info_pluggable_.end(),
                   [&client_id](auto &i) { return client_id == i.client_id; });

  // return connected as true for all non pluggable displays
  if (itr_map == map_info_pluggable_.end()) {
    return true;
  }

  auto sdm_id = itr_map->sdm_id;

  auto itr_hw = std::find_if(
      hw_displays_info.begin(), hw_displays_info.end(),
      [&sdm_id](auto &info) { return sdm_id == info.second.display_id; });

  if (itr_hw == hw_displays_info.end()) {
    DLOGW("client id: %d, sdm_id: %d not found in hw map", client_id, sdm_id);
    return false;
  }

  if (!itr_hw->second.is_connected) {
    DLOGW("client_id: %d, sdm_id: %d, not connected", client_id, sdm_id);
    return false;
  }

  DLOGI("client_id: %d, sdm_id: %d, is connected", client_id, sdm_id);
  return true;
}

int SDMDisplayBuilder::HandlePluggableDisplays(bool delay_hotplug) {
  SCOPE_LOCK(locker_[pluggable_lock_index_]);
  uint64_t virtual_display_index = (uint64_t)GetDisplayIndex(qdutilsDisplayType::DISPLAY_VIRTUAL);
  std::bitset<kSecureMax> secure_sessions = 0;

  uint64_t active_builtin_disp_id = GetActiveBuiltinDisplay();
  auto disp = cb_->GetDisplayFromClientId(active_builtin_disp_id);

  if (active_builtin_disp_id < kNumDisplays) {
    Locker::ScopeLock lock_a(locker_[active_builtin_disp_id]);
    disp->GetActiveSecureSession(&secure_sessions);
  }

  if (secure_sessions.any() ||
      cb_->GetDisplayFromClientId(virtual_display_index)) {
    // Defer hotplug handling.
    DLOGI("Marking hotplug pending...");
    pending_hotplug_event_ = kHotPlugEvent;
    return -EAGAIN;
  }

  DLOGI("Handling hotplug...");
  HWDisplaysInfo hw_displays_info = {};
  DisplayError error = core_intf_->GetDisplaysStatus(&hw_displays_info);
  if (error != kErrorNone) {
    DLOGW("Failed to get connected display list. Error = %d", error);
    return -EINVAL;
  }

  int status = HandleDisconnectedDisplays(&hw_displays_info);
  if (status) {
    DLOGE("All displays could not be disconnected.");
    return status;
  }

  status = HandleConnectedDisplays(&hw_displays_info, delay_hotplug);
  if (status) {
    switch (status) {
    case -EAGAIN:
    case -ENODEV:
      // Errors like device removal or deferral for which we want to try another
      // hotplug handling.
      pending_hotplug_event_ = kHotPlugEvent;

      if (active_builtin_disp_id < kNumDisplays) {
        if (delay_hotplug) {
          cb_->WaitForCommitDone(active_builtin_disp_id, kClientTrustedUI);
        } else {
          callbacks_->OnRefresh(active_builtin_disp_id);
        }
      }

      status = 0;
      break;
    default:
      // Real errors we want to flag and stop hotplug handling.
      pending_hotplug_event_ = kHotPlugNone;
      DLOGE("All displays could not be connected. Error %d '%s'.", status,
            strerror(abs(status)));
    }
    DLOGI("Handling hotplug... %s", (kHotPlugNone == pending_hotplug_event_)
                                        ? "Stopped."
                                        : "Done. Hotplug events pending.");
    return status;
  }

  pending_hotplug_event_ = kHotPlugNone;
  if (active_builtin_disp_id < kNumDisplays && delay_hotplug) {
    cb_->WaitForCommitDone(active_builtin_disp_id, kClientTrustedUI);
  }

  DLOGI("Handling hotplug... Done.");
  return 0;
}

void SDMDisplayBuilder::HandlePluggableDisplaysAsync(
    const shared_ptr<Fence> &retire_fence) {
  if (kHotPlugEvent != pending_hotplug_event_) {
    return;
  }

  if (retire_fence) {
    Fence::Wait(retire_fence);
  }

  std::thread(&SDMDisplayBuilder::HandlePluggableDisplays, this, true).detach();
}

int SDMDisplayBuilder::HandleConnectedDisplays(HWDisplaysInfo *displays_info,
                                               bool delay_hotplug) {
  int status = 0;
  Display client_id = 0;

  for (auto &iter : *displays_info) {
    auto &info = iter.second;

    // Do not recreate primary display or if display is not connected.
    if (info.is_primary || info.display_type != kPluggable ||
        !info.is_connected) {
      continue;
    }

    // Check if we are already using the display.
    auto display_used = std::find_if(
        map_info_pluggable_.begin(), map_info_pluggable_.end(),
        [&](auto &p) { return (p.sdm_id == info.display_id); }); // NOLINT
    if (display_used != map_info_pluggable_.end()) {
      // Display is already used in a slot.
      continue;
    }

    // Count active pluggable display slots and slots with no commits.
    bool first_commit_pending = false;
    std::for_each(map_info_pluggable_.begin(), map_info_pluggable_.end(),
                  [&](auto &p) { // NOLINT
                    auto disp = cb_->GetDisplayFromClientId(p.client_id);
                    if (disp) {
                      if (!disp->IsFirstCommitDone()) {
                        DLOGI("Display commit pending on display %d-1",
                              p.sdm_id);
                        first_commit_pending = true;
                      }
                    }
                  });

    if (!disable_hotplug_bwcheck_ && first_commit_pending) {
      // Hotplug bandwidth check is accomplished by creating and hotplugging a
      // new display after a display commit has happened on previous hotplugged
      // displays. This allows the driver to return updated modes for the new
      // display based on available link bandwidth.
      DLOGI("Pending display commit on one of the displays. Deferring display "
            "creation.");
      status = -EAGAIN;
      if (cb_->IsClientConnected()) {
        // Trigger a display refresh since we depend on PresentDisplay() to
        // handle pending hotplugs.
        Display active_builtin_disp_id = GetActiveBuiltinDisplay();
        if (active_builtin_disp_id >= kNumDisplays) {
          active_builtin_disp_id = SDM_DISPLAY_PRIMARY;
        }
        callbacks_->OnRefresh(active_builtin_disp_id);
      }
      break;
    }

    int hpd_bpp = 0;
    int hpd_pattern = 0;
    int hpd_connected = 0;

    cb_->GetHpdData(&hpd_bpp, &hpd_pattern, &hpd_connected);

    // find an empty slot to create display.
    for (auto &map_info : map_info_pluggable_) {
      client_id = map_info.client_id;

      auto sdm_display = cb_->GetDisplayFromClientId(client_id);
      if (sdm_display) {
        // Display slot is already used.
        continue;
      }

      DLOGI("Create pluggable display, sdm id = %d, client id = %d",
            info.display_id, UINT32(client_id));

      // Test pattern generation ?
      map_info.test_pattern = (hpd_bpp > 0) && (hpd_pattern > 0);
      int err = 0;
      if (!map_info.test_pattern) {
        err = SDMDisplayPluggable::Create(
            core_intf_, buffer_allocator_, callbacks_, evt_handler_, client_id,
            info.display_id, 0, 0, false, &sdm_display);
      } else {
        err = SDMDisplayPluggableTest::Create(
            core_intf_, buffer_allocator_, callbacks_, evt_handler_, client_id,
            info.display_id, UINT32(hpd_bpp), UINT32(hpd_pattern),
            &sdm_display);
      }

      if (err) {
        DLOGW("Pluggable display creation failed/aborted. Error %d '%s'.", err,
              strerror(abs(err)));
        status = err;
        // Attempt creating remaining pluggable displays.
        break;
      }

      {
        SCOPE_LOCK(hdr_locker_[client_id]);
        is_hdr_display_[UINT32(client_id)] = HasHDRSupport(sdm_display);
      }

      DLOGI(
          "Created pluggable display successfully: sdm id = %d, client id = %d",
          info.display_id, UINT32(client_id));

      map_info.disp_type = info.display_type;
      map_info.sdm_id = info.display_id;

      map_active_displays_.insert(std::make_pair(client_id, &map_info));
      cb_->SetDisplayByClientId(client_id, sdm_display);

      pending_hotplugs_.push_back((Display)client_id);

      // Display is created for this sdm id, move to next connected display.
      break;
    }
  }

  // No display was created.
  if (!pending_hotplugs_.size()) {
    return status;
  }

  // Active builtin display needs revalidation
  Display active_builtin_disp_id = GetActiveBuiltinDisplay();
  if (active_builtin_disp_id < kNumDisplays) {
    auto ret =
        cb_->WaitForResources(delay_hotplug, active_builtin_disp_id, client_id);
    if (ret != kErrorNone) {
      return -EAGAIN;
    }
  }

  for (auto client_id : pending_hotplugs_) {
    DLOGI("Notify hotplug display connected: client id = %d",
          UINT32(client_id));
    callbacks_->OnHotplug(client_id, true);
  }

  pending_hotplugs_.clear();

  return status;
}

bool SDMDisplayBuilder::TeardownPluggableDisplays() {
  bool hpd_teardown_handled = false;

  while (true) {
    auto it = std::find_if(
        map_active_displays_.begin(), map_active_displays_.end(),
        [](auto &disp) { return disp.second->disp_type == kPluggable; });

    if (it == map_active_displays_.end()) {
      break;
    }

    hpd_teardown_handled |= !DisconnectPluggableDisplays(it->second);
  }

  if (hpd_teardown_handled) {
    pending_hotplug_event_ = kHotPlugEvent;
  }

  return hpd_teardown_handled;
}

int SDMDisplayBuilder::HandleDisconnectedDisplays(
    HWDisplaysInfo *hw_displays_info) {
  // Destroy pluggable displays which were connected earlier but got
  // disconnected now.
  for (auto &map_info : map_info_pluggable_) {
    bool disconnect =
        true; // disconnect in case display id is not found in list.

    for (auto &iter : *hw_displays_info) {
      auto &info = iter.second;
      if (info.display_id != map_info.sdm_id) {
        continue;
      }

      if (info.is_connected) {
        disconnect = false;
      }
      break;
    }

    if (!disconnect) {
      continue;
    }

    DisconnectPluggableDisplays(&map_info);
  }

  return 0;
}

int SDMDisplayBuilder::DisconnectPluggableDisplays(DisplayMapInfo *map_info) {
  Display client_id = map_info->client_id;
  bool is_valid_pluggable_display = false;
  auto sdm_display = cb_->GetDisplayFromClientId(client_id);
  if (sdm_display) {
    is_valid_pluggable_display = true;
    sdm_display->Abort();
  }

  DestroyDisplay(map_info);

  if (enable_primary_reconfig_req_ && is_valid_pluggable_display) {
    Display active_builtin_id = GetActiveBuiltinDisplay();
    auto disp = cb_->GetDisplayFromClientId(active_builtin_id);

    if (active_builtin_id < kNumDisplays && disp) {
      SCOPE_LOCK(locker_[active_builtin_id]);
      Config current_config = 0, new_config = 0;
      disp->GetActiveConfig(false, &current_config);
      disp->SetAlternateDisplayConfig(false);
      disp->GetActiveConfig(false, &new_config);

      if (new_config != current_config) {
        cb_->NotifyDisplayAttributes(active_builtin_id, new_config);
      }
    }
  }

  auto id =
      std::find(pending_hotplugs_.begin(), pending_hotplugs_.end(), client_id);
  if (id != pending_hotplugs_.end()) {
    pending_hotplugs_.erase(id);
  }
  return 0;
}

void SDMDisplayBuilder::DestroyDisplay(DisplayMapInfo *map_info) {
  switch (map_info->disp_type) {
  case kPluggable: {
    DLOGI("Notify hotplug display disconnected: client id = %d",
          UINT32(map_info->client_id));
    callbacks_->OnHotplug(map_info->client_id, false);

    // Wait until all commands are flushed.
    std::lock_guard<std::mutex> sdm_lock(cb_->command_seq_mutex_);

    cb_->SetPowerMode(map_info->client_id,
                      static_cast<int32_t>(SDMPowerMode::POWER_MODE_OFF));
    DestroyPluggableDisplay(map_info);
    break;
  }
  default:
    DestroyNonPluggableDisplay(map_info);
    break;
  }
}

void SDMDisplayBuilder::DestroyDisplayLocked(int display_id) {
  auto &map_info = GetDisplayMapInfo(display_id)[0];
  if (map_info.sdm_id == -1) {
    return;
  }

  switch (map_info.disp_type) {
  case kPluggable: {
    DLOGI("Notify hotplug display disconnected: client id = %d",
          UINT32(map_info.client_id));
    callbacks_->OnHotplug(map_info.client_id, false);
    cb_->SetPowerMode(map_info.client_id,
                      static_cast<int32_t>(SDMPowerMode::POWER_MODE_OFF));
    DestroyPluggableDisplayLocked(&map_info);
    break;
  }
  default:
    DestroyNonPluggableDisplayLocked(&map_info);
    break;
  }
}

void SDMDisplayBuilder::DestroyPluggableDisplay(DisplayMapInfo *map_info) {
  SCOPE_LOCK(locker_[map_info->client_id]);

  DestroyPluggableDisplayLocked(map_info);
}

void SDMDisplayBuilder::DestroyPluggableDisplayLocked(
    DisplayMapInfo *map_info) {
  Display client_id = map_info->client_id;

  auto sdm_display = cb_->GetDisplayFromClientId(client_id);
  if (!sdm_display) {
    return;
  }
  DLOGI("Destroy display %d-%d, client id = %d", map_info->sdm_id,
        map_info->disp_type, UINT32(client_id));

  {
    SCOPE_LOCK(hdr_locker_[client_id]);
    is_hdr_display_[UINT32(client_id)] = false;
  }

  if (!map_info->test_pattern) {
    SDMDisplayPluggable::Destroy(sdm_display);
  } else {
    SDMDisplayPluggableTest::Destroy(sdm_display);
  }

  map_active_displays_.erase(client_id);
  cb_->SetDisplayByClientId(client_id, nullptr);
  map_info->Reset();
}

void SDMDisplayBuilder::DestroyNonPluggableDisplay(DisplayMapInfo *map_info) {
  SCOPE_LOCK(locker_[map_info->client_id]);

  DestroyNonPluggableDisplayLocked(map_info);
}

void SDMDisplayBuilder::DestroyNonPluggableDisplayLocked(
    DisplayMapInfo *map_info) {
  Display client_id = map_info->client_id;

  auto sdm_display = cb_->GetDisplayFromClientId(client_id);
  if (!sdm_display) {
    return;
  }
  DLOGI("Destroy display %d-%d, client id = %d", map_info->sdm_id,
        map_info->disp_type, UINT32(client_id));

  {
    SCOPE_LOCK(hdr_locker_[client_id]);
    is_hdr_display_[UINT32(client_id)] = false;
  }

  switch (map_info->disp_type) {
  case kBuiltIn:
    SDMDisplayBuiltIn::Destroy(sdm_display);
    break;
  default:
    virtual_display_factory_.Destroy(sdm_display);
    break;
  }

  map_active_displays_.erase(client_id);

  cb_->SetDisplayByClientId(client_id, nullptr);
  map_info->Reset();
}

void SDMDisplayBuilder::RemoveDisconnectedPluggableDisplays() {
  SCOPE_LOCK(locker_[pluggable_lock_index_]);
  HWDisplaysInfo hw_displays_info = {};
  DisplayError error = core_intf_->GetDisplaysStatus(&hw_displays_info);
  if (error != kErrorNone) {
    return;
  }

  HandleDisconnectedDisplays(&hw_displays_info);
}

void SDMDisplayBuilder::SetLuminance(float min_lum, float max_lum) {
  set_min_lum_ = min_lum;
  set_max_lum_ = max_lum;
}

bool SDMDisplayBuilder::IsBuiltInDisplay(uint64_t disp_id) {
  auto &map_primary = GetDisplayMapInfo(qdutilsDisplayType::DISPLAY_PRIMARY)[0];
  if ((map_primary.client_id == disp_id) &&
      (map_primary.disp_type == kBuiltIn)) {
    return true;
  }

  for (auto &info : GetDisplayMapInfo(qdutilsDisplayType::DISPLAY_BUILTIN_2)) {
    if (disp_id == info.client_id) {
      return true;
    }
  }

  return false;
}

DisplayError SDMDisplayBuilder::GetDisplayHwId(uint64_t disp_id,
                                               int32_t *disp_hw_id) {
  int disp_idx = GetDisplayIndex(disp_id);
  if (disp_idx == -1) {
    DLOGE("Invalid display = %d", disp_id);
    return kErrorNotSupported;
  }

  SCOPE_LOCK(locker_[disp_id]);
  if (!cb_->GetDisplayFromClientId(disp_idx)) {
    DLOGE("Display %d is not connected.", disp_id);
    return kErrorNotSupported;
  }

  // Supported for Built-In displays only.
  auto &map_info = GetDisplayMapInfo(qdutilsDisplayType::DISPLAY_PRIMARY)[0];
  if ((map_info.client_id == disp_id) &&
      (map_info.disp_type == sdm::kBuiltIn)) {
    if (map_info.sdm_id >= 0) {
      uint32_t base_core_id = DisplayId::GetBaseCoreId(map_info.sdm_id);
      uint32_t conn_id = DisplayId::GetConnId(map_info.sdm_id, base_core_id);
      *disp_hw_id = conn_id;
      return kErrorNone;
    }
  }

  for (auto &info : GetDisplayMapInfo(qdutilsDisplayType::DISPLAY_BUILTIN_2)) {
    if (disp_id == info.client_id) {
      if (info.sdm_id >= 0) {
        *disp_hw_id = static_cast<uint32_t>(info.sdm_id);
        return kErrorNone;
      }
    }
  }

  return kErrorNotSupported;
}

} // namespace sdm
