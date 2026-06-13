/*
 * Copyright (c) 2014-2021, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
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
 * Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include <algorithm>
#include <vector>

#include "sdm_services.h"
#include "sdm_tui.h"

#define __CLASS__ "SDMTrustedUI"

namespace sdm {

Locker SDMTrustedUI::vm_release_locker_[kNumDisplays];
std::bitset<kNumDisplays> SDMTrustedUI::clients_waiting_for_vm_release_;

void SDMTrustedUI::Init(SDMDisplayBuilder *disp, Locker *locker,
                        int pluggable_lock_index) {
  disp_ = disp;
  locker_ = locker;
  pluggable_lock_index_ = pluggable_lock_index;
}

int SDMTrustedUI::NotifyTUIEventDone(int disp_id, SDMTUIEventType event_type) {
  int ret = 0;
  {
    std::lock_guard<std::mutex> guard(tui_handler_lock_);
    if (tui_event_handler_future_.valid()) {
      ret = tui_event_handler_future_.get();
    }
  }

  cb_->NotifyTUIDone(ret, disp_id, event_type);
  return kErrorNone;
}

DisplayError SDMTrustedUI::HandleTUITransition(int disp_id, int event) {
  switch (event) {
  case SDM_SERVICE_TUI_TRANSITION_PREPARE:
    return TUIEventHandler(disp_id, SDMTUIEventType::PREPARE_TUI_TRANSITION);
  case SDM_SERVICE_TUI_TRANSITION_START:
    return TUIEventHandler(disp_id, SDMTUIEventType::START_TUI_TRANSITION);
  case SDM_SERVICE_TUI_TRANSITION_END:
    return TUIEventHandler(disp_id, SDMTUIEventType::END_TUI_TRANSITION);
  default:
    DLOGE("Invalid event %d", event);
    return kErrorNotSupported;
  }
}

DisplayError SDMTrustedUI::TUIEventHandler(int disp_id,
                                           SDMTUIEventType event_type) {
  std::lock_guard<std::mutex> guard(tui_handler_lock_);
  if (tui_event_handler_future_.valid()) {
    std::future_status status =
        tui_event_handler_future_.wait_for(std::chrono::milliseconds(0));
    if (status != std::future_status::ready) {
      DLOGW("Event handler thread is busy with previous work!!");
      return kErrorDeviceBusy;
    }
  }
  switch (event_type) {
  case SDMTUIEventType::PREPARE_TUI_TRANSITION:
    tui_event_handler_future_ =
        std::async([](SDMTrustedUI *tui, int disp_id) { return kErrorNone; },
                   this, disp_id);
    break;
  case SDMTUIEventType::START_TUI_TRANSITION:
    tui_event_handler_future_ =
        std::async([](SDMTrustedUI *tui,
                      int disp_id) { return tui->TUITransitionStart(disp_id); },
                   this, disp_id);
    break;
  case SDMTUIEventType::END_TUI_TRANSITION:
    tui_event_handler_future_ =
        std::async([](SDMTrustedUI *tui,
                      int disp_id) { return tui->TUITransitionEnd(disp_id); },
                   this, disp_id);
    break;
  default:
    DLOGE("Invalid event %d", event_type);
    return kErrorNotSupported;
  }
  if (tui_callback_handler_future_.valid()) {
    std::future_status status =
        tui_callback_handler_future_.wait_for(std::chrono::milliseconds(1000));
    if (status != std::future_status::ready) {
      DLOGW("callback handler thread is busy with previous work!!");
      return kErrorDeviceBusy;
    }
  }
  tui_callback_handler_future_ = std::async(
      [](SDMTrustedUI *tui, int disp_id, SDMTUIEventType event_type) {
        return tui->NotifyTUIEventDone(disp_id, event_type);
      },
      this, disp_id, event_type);
  return kErrorNone;
}

DisplayError SDMTrustedUI::TUITransitionPrepare(int disp_id) {
  bool needs_refresh = false;
  Display target_display = disp_->GetDisplayIndex(disp_id);
  if (target_display == -1) {
    target_display = disp_->GetActiveBuiltinDisplay();
  }

  if (target_display != qdutilsDisplayType::DISPLAY_PRIMARY &&
      target_display != qdutilsDisplayType::DISPLAY_BUILTIN_2) {
    DLOGE("Display %" PRIu64 " not supported", target_display);
    return kErrorNotSupported;
  }

  std::bitset<kSecureMax> secure_sessions = 0;
  SDMPowerMode current_power_mode = SDMPowerMode::POWER_MODE_OFF;
  {
    SEQUENCE_WAIT_SCOPE_LOCK(locker_[target_display]);
    auto display = cb_->GetDisplayFromClientId(target_display);
    if (display) {
      display->GetActiveSecureSession(&secure_sessions);
      current_power_mode = display->GetCurrentPowerMode();
    }
  }

  if (current_power_mode != SDMPowerMode::POWER_MODE_ON) {
    DLOGW("TUI session not allowed as target display is not powered On");
    return kErrorNotSupported;
  }

  if (secure_sessions[kSecureCamera]) {
    DLOGW("TUI session not allowed during ongoing Secure Camera session");
    return kErrorNotSupported;
  }

  std::vector<DisplayMapInfo> map_info = {
      disp_->GetDisplayMapInfo(qdutilsDisplayType::DISPLAY_PRIMARY)[0]};
  auto &map_info_builtin = disp_->GetDisplayMapInfo(qdutilsDisplayType::DISPLAY_BUILTIN_2);
  auto &map_info_virtual = disp_->GetDisplayMapInfo(qdutilsDisplayType::DISPLAY_VIRTUAL);

  std::copy(map_info_builtin.begin(), map_info_builtin.end(),
            std::back_inserter(map_info));
  std::copy(map_info_virtual.begin(), map_info_virtual.end(),
            std::back_inserter(map_info));

  for (auto &info : map_info) {
    SEQUENCE_WAIT_SCOPE_LOCK(locker_[info.client_id]);
    auto display = cb_->GetDisplayFromClientId(info.client_id);
    if (!display) {
      continue;
    }

    if (display->HandleSecureEvent(kTUITransitionPrepare, &needs_refresh,
                                   info.client_id == target_display) !=
        kErrorNone) {
      return kErrorNotSupported;
    }
  }

  disp_->TeardownPluggableDisplays();

  return kErrorNone;
}

DisplayError SDMTrustedUI::TUITransitionStart(int disp_id) {
  // Hold this lock to until on going hotplug handling is complete before we
  // start TUI session
  SCOPE_LOCK(locker_[pluggable_lock_index_]);
  if (TUITransitionPrepare(disp_id) != 0) {
    return kErrorNotSupported;
  }

  Display target_display = disp_->GetDisplayIndex(disp_id);
  bool needs_refresh = false;

  DisplayError error = cb_->TeardownConcurrentWriteback(target_display);
  if (error != kErrorNone) {
    return kErrorResources;
  }

  {
    std::lock_guard<std::mutex> tui_lock(cb_->tui_mutex_);
    SCOPE_LOCK(locker_[target_display]);

    // disable idle time out for video mode
    auto display = cb_->GetDisplayFromClientId(target_display);
    display->SetIdleTimeoutMs(0, 0);

    // disable qsync
    display->SetQSyncMode(kQSyncModeNone);
  }

  int timeout_ms = -1;
  {
    SEQUENCE_WAIT_SCOPE_LOCK(locker_[target_display]);
    auto display = cb_->GetDisplayFromClientId(target_display);
    DisplayError err = kErrorNone;

    if (display) {
      if ((err = display->HandleSecureEvent(kTUITransitionStart, &needs_refresh,
                                            false)) != kErrorNone) {
        if (err == kErrorPermission) {
          DLOGW("Bail from Start. Call unprepare");
          goto end;
        }
        return kErrorNotSupported;
      }
      uint32_t config = 0;
      display->GetActiveDisplayConfig(false, &config);
      DisplayConfigVariableInfo display_attributes = {};
      display->GetDisplayAttributesForConfig(config, &display_attributes);
      timeout_ms =
          kNumDrawCycles * (display_attributes.vsync_period_ns / kDenomNstoMs);
      DLOGI("timeout in ms %d", timeout_ms);
    } else {
      DLOGW("Target display %d is not ready", disp_id);
      return kErrorResources;
    }
  }

  if (needs_refresh) {
    cb_->Refresh(target_display);

    DLOGI("Waiting for device assign");
    auto ret = WaitForVmRelease(target_display, timeout_ms);
    if (ret == kErrorHardware) {
      DLOGW("Unwind TUI");
      TUITransitionEndLocked(target_display);
      return ret;
    }
    if (ret != kErrorNone) {
      DLOGE("Device assign failed with error %d", ret);
      return ret;
    }
  }

  {
    SEQUENCE_WAIT_SCOPE_LOCK(locker_[target_display]);
    auto display = cb_->GetDisplayFromClientId(target_display);
    if (display) {
      if (display->PostHandleSecureEvent(kTUITransitionStart) != kErrorNone) {
        return kErrorNotSupported;
      }
    } else {
      DLOGW("Target display %d is not ready", disp_id);
      return kErrorResources;
    }
  }

  return kErrorNone;

end:
  TUITransitionUnPrepare(disp_id);
  return kErrorPermission;
}

DisplayError SDMTrustedUI::TUITransitionEnd(int disp_id) {
  // Hold this lock so that any deferred hotplug events will not be handled
  // during the commit and will be handled at the end of TUITransitionPrepare.
  SCOPE_LOCK(locker_[pluggable_lock_index_]);
  return TUITransitionEndLocked(disp_id);
}

void SDMTrustedUI::SetIdleTimeoutMs(uint32_t value, uint32_t inactive_ms) {
  idle_time_active_ms_ = value;
  idle_time_inactive_ms_ = inactive_ms;
}

DisplayError SDMTrustedUI::TUITransitionEndLocked(int disp_id) {
  Display target_display = disp_->GetDisplayIndex(disp_id);
  bool needs_refresh = false;
  if (target_display == -1) {
    target_display = disp_->GetActiveBuiltinDisplay();
  }

  if (target_display != qdutilsDisplayType::DISPLAY_PRIMARY &&
      target_display != qdutilsDisplayType::DISPLAY_BUILTIN_2) {
    DLOGE("Display %" PRIu64 " not supported", target_display);
    return kErrorNotSupported;
  }

  {
    SEQUENCE_WAIT_SCOPE_LOCK(locker_[target_display]);
    SDMDisplay *display = cb_->GetDisplayFromClientId(target_display);
    if (!display) {
      DLOGW("Target display %d is not ready", disp_id);
      return kErrorResources;
    }

    display->SetIdleTimeoutMs(idle_time_active_ms_, idle_time_inactive_ms_);
    display->SetQSyncMode(sdm_display_qsync_[target_display]);

    auto ret =
        display->HandleSecureEvent(kTUITransitionEnd, &needs_refresh, false);
    if (ret != kErrorNone) {
      return kErrorNotSupported;
    }
  }

  // Add check for internal state for bailing out (needs_refresh to false)
  if (needs_refresh) {
    DLOGI("Waiting for device unassign");
    DisplayError ret =
        cb_->WaitForCommitDoneAsync(target_display, kClientTrustedUI);
    if (ret != 0) {
      if (ret != kErrorTimeOut) {
        DLOGE("Device unassign failed with error %d", ret);
      }
      TUITransitionUnPrepare(disp_id);
      return kErrorNotSupported;
    }
  }

  {
    SEQUENCE_WAIT_SCOPE_LOCK(locker_[target_display]);
    SDMDisplay *display = cb_->GetDisplayFromClientId(target_display);
    if (display) {
      if (display->PostHandleSecureEvent(kTUITransitionEnd) != kErrorNone) {
        return kErrorNotSupported;
      }
    } else {
      DLOGW("Target display %d is not ready", disp_id);
      return kErrorResources;
    }
  }

  return TUITransitionUnPrepare(disp_id);
}

DisplayError SDMTrustedUI::TUITransitionUnPrepare(int disp_id) {
  bool trigger_refresh = false;
  Display target_display = disp_->GetDisplayIndex(disp_id);
  if (target_display == -1) {
    target_display = disp_->GetActiveBuiltinDisplay();
  }

  if (target_display != qdutilsDisplayType::DISPLAY_PRIMARY &&
      target_display != qdutilsDisplayType::DISPLAY_BUILTIN_2) {
    DLOGE("Display %" PRIu64 " not supported", target_display);
    return kErrorNotSupported;
  }

  std::vector<DisplayMapInfo> map_info = {
      disp_->GetDisplayMapInfo(qdutilsDisplayType::DISPLAY_PRIMARY)[0]};
  auto &map_info_builtin = disp_->GetDisplayMapInfo(qdutilsDisplayType::DISPLAY_BUILTIN_2);
  auto &map_info_virtual = disp_->GetDisplayMapInfo(qdutilsDisplayType::DISPLAY_VIRTUAL);

  std::copy(map_info_builtin.begin(), map_info_builtin.end(),
            std::back_inserter(map_info));
  std::copy(map_info_virtual.begin(), map_info_virtual.end(),
            std::back_inserter(map_info));

  for (auto &info : map_info) {
    bool needs_refresh = false;
    {
      SEQUENCE_WAIT_SCOPE_LOCK(locker_[info.client_id]);
      SDMDisplay *display = cb_->GetDisplayFromClientId(info.client_id);
      if (display) {
        if (display->HandleSecureEvent(kTUITransitionUnPrepare, &needs_refresh,
                                       info.client_id == target_display) !=
            kErrorNone) {
          return kErrorNotSupported;
        }
      }
      trigger_refresh |= needs_refresh;
    }
  }
  if (trigger_refresh) {
    cb_->Refresh(target_display);
  }

  disp_->HandlePluggableDisplaysAsync();

  // Reset tui session state variable.
  DLOGI("End of TUI session on display %d", disp_id);
  return kErrorNone;
}

DisplayError SDMTrustedUI::WaitForVmRelease(Display disp_id, int timeout_ms) {
  SCOPE_LOCK(vm_release_locker_[disp_id]);

  clients_waiting_for_vm_release_.set(disp_id);
  int re_try = kVmReleaseRetry;
  int ret = 0;
  do {
    auto display = cb_->GetDisplayFromClientId(disp_id);
    if (display->GetCurrentPowerMode() == SDMPowerMode::POWER_MODE_OFF) {
      return kErrorHardware;
    }
    ret = vm_release_locker_[disp_id].WaitFinite(timeout_ms +
                                                 kVmReleaseTimeoutMs);
    if (!ret) {
      break;
    }
  } while (re_try--);
  if (ret != 0) {
    DLOGE("Timed out with error %d for display %" PRIu64, ret, disp_id);
  }
  return ret == 0 ? kErrorNone : kErrorTimeOut;
}

void SDMTrustedUI::VmReleaseDone(Display display) {
  SCOPE_LOCK(vm_release_locker_[display]);

  if (clients_waiting_for_vm_release_.test(display)) {
    vm_release_locker_[display].Signal();
    DLOGI("Signal vm release done!! for display %d", display);
    clients_waiting_for_vm_release_.reset(display);
  }
}

} // namespace sdm
