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
#ifndef __SDM_TUI_H__
#define __SDM_TUI_H__

#include <utils/locker.h>

#include <future> // NOLINT
#include <vector>

#include "sdm_display_builder.h"
#include "sdm_tui_cb_intf.h"

namespace sdm {

class SDMTrustedUI {
public:
  explicit SDMTrustedUI(SDMTrustedUICbIntf *cb) : cb_(cb) {}

  void Init(SDMDisplayBuilder *disp, Locker *locker, int pluggable_lock_index);
  DisplayError HandleTUITransition(int disp_id, int event);
  void VmReleaseDone(Display display);
  int NotifyTUIEventDone(int disp_id, SDMTUIEventType event_type);
  DisplayError TUIEventHandler(int disp_id, SDMTUIEventType event_type);
  void SetIdleTimeoutMs(uint32_t value, uint32_t inactive_ms);
  void SetBackendQsyncMode(QSyncMode qsync_mode) {
    sdm_display_qsync_[SDM_DISPLAY_PRIMARY] = qsync_mode;
  }

private:
  DisplayError TUITransitionPrepare(int disp_id);
  DisplayError TUITransitionStart(int disp_id);
  DisplayError TUITransitionEnd(int disp_id);
  DisplayError TUITransitionEndLocked(int disp_id);
  DisplayError TUITransitionUnPrepare(int disp_id);
  DisplayError WaitForVmRelease(Display display, int timeout_ms);

  SDMTrustedUICbIntf *cb_ = nullptr;
  SDMDisplayBuilder *disp_ = nullptr;
  Locker *locker_ = nullptr;

  std::mutex tui_handler_lock_;
  std::future<DisplayError> tui_event_handler_future_;
  std::future<int> tui_callback_handler_future_;

  static Locker vm_release_locker_[kNumDisplays];
  static std::bitset<kNumDisplays> clients_waiting_for_vm_release_;
  static const int kVmReleaseTimeoutMs = 100;
  static const int kVmReleaseRetry = 3;
  static const int kDenomNstoMs = 1000000;
  static const int kNumDrawCycles = 3;
  int pluggable_lock_index_ = 0;
  uint32_t idle_time_active_ms_ = 0;
  uint32_t idle_time_inactive_ms_ = 0;
  QSyncMode sdm_display_qsync_[kNumDisplays] = {QSyncMode::kQSyncModeNone};
};

} // namespace sdm

#endif // __SDM_TUI_H__
