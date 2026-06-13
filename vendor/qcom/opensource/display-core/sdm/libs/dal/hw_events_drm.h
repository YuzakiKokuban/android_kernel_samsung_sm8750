/*
* Copyright (c) 2017-2018, 2020-2021 The Linux Foundation. All rights reserved.
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
* Changes from Qualcomm Innovation Center are provided under the following license:
* Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#ifndef __HW_EVENTS_DRM_H__
#define __HW_EVENTS_DRM_H__

#include <drm_interface.h>
#include <sys/poll.h>
#include <sys/inotify.h>
#include <private/hw_events_interface.h>
#include <private/hw_interface.h>
#include <map>
#include <mutex>
#include <string>
#include <utility>
#include <vector>
#include <climits>
#include <bitset>

#include "hw_device_drm.h"

namespace sdm {

using std::vector;

class HWEventsDRM : public HWEventsInterface {
 public:
  virtual DisplayError Init(DisplayId display_id, SDMDisplayType display_type,
                            HWEventHandler *event_handler, const vector<HWEvent> &event_list);
  virtual DisplayError Deinit();
  virtual DisplayError SetEventState(HWEvent event, bool enable, void *aux = nullptr);
#ifdef SEC_GC_QC_VSYNC
  virtual DisplayError GetEventState (bool *enabled, bool *registered, int64_t *timestamp, int64_t *fail_timestamp, int *error);
#endif

 private:
  static const int kMaxStringLength = 1024;
  static const int kMaxEventBufferLength = (kMaxStringLength * (sizeof(struct inotify_event) + 16));

  typedef void (HWEventsDRM::*EventParser)(char *);

  struct HWEventData {
    HWEvent event_type {};
    EventParser event_parser {};
  };

  static void *DisplayEventThread(void *context);
  static void VSyncHandlerCallback(int fd, unsigned int sequence, unsigned int tv_sec,
                                   unsigned int tv_usec, void *data);

  void *DisplayEventHandler();
  void HandleVSync(char *data);
  void HandleCECMessage(char *data);
  void HandleThreadExit(char *data) {}
  void HandleThermal(char *data) {}
  void HandleBlank(char *data) {}
  void HandleIdlePowerCollapse(char *data);
  void HandlePanelDead(char *data);
  void HandleHwRecovery(char *data);
  void HandleHistogram(char *data);
  void HandleBacklightEvent(char *data);
  void HandleMMRM(char *data);
  void HandlePowerEvent(char * /*data*/);
  void HandleVmReleaseEvent(char * /*data*/);
#ifdef SEC_GC_QC_DYN_CLK
  void HandleDynamicClockEvent(char *data);
#endif
  int SetHwRecoveryEvent(const uint32_t hw_event_code, HWRecoveryEvent *sdm_event_code);
  void PopulateHWEventData(const vector<HWEvent> &event_list);
  void WakeUpEventThread();
  DisplayError SetEventParser();
  DisplayError InitializePollFd();
  void CloseFds();
  DisplayError RegisterVSync();
  DisplayError RegisterPanelDead(bool enable);
  DisplayError RegisterIdlePowerCollapse(bool enable);
  DisplayError RegisterHwRecovery(bool enable);
  DisplayError RegisterHistogram(bool enable);
  DisplayError RegisterMMRM(bool enable);
  DisplayError RegisterPowerEvents(bool enable);
  DisplayError RegisterVmReleaseEvents(bool enable);
#ifdef SEC_GC_QC_DYN_CLK
  DisplayError RegisterDynamicClockEvent(bool enable);
#endif
  void HandleDRMOpen(int& fd);

  HWEventHandler *event_handler_{};
  vector<HWEventData> event_data_list_{};
  vector<pollfd> poll_fds_{};
  pthread_t event_thread_{};
  std::string event_thread_name_ = "SDM_EventThread";
  bool exit_threads_ = false;
  uint32_t vsync_index_ = UINT32_MAX;
  uint32_t histogram_index_ = UINT32_MAX;
#ifdef SEC_GC_QC_DYN_CLK
  uint32_t dynclock_index_ = UINT32_MAX;
#endif
  bool vsync_enabled_ = false;
  uint32_t vsync_handler_count_ = 0;
  std::mutex vsync_mutex_;  // To protect vsync_enabled_
  sde_drm::DRMDisplayToken token_ = {};
  bool is_primary_ = false;
  uint32_t panel_dead_index_ = UINT32_MAX;
  uint32_t idle_pc_index_ = UINT32_MAX;
  bool disable_hw_recovery_ = false;
  bool enable_hist_interrupt_ = false;
  uint32_t hw_recovery_index_ = UINT32_MAX;
  std::mutex backlight_mutex_;
  uint32_t backlight_event_index_ = UINT32_MAX;
  std::string brightness_node_ = {};
  int backlight_wd_ = -1;
  bool disable_mmrm_ = false;
  uint32_t mmrm_index_ = UINT32_MAX;
  uint32_t power_event_index_ = UINT32_MAX;
  uint32_t vm_release_event_index_ = UINT32_MAX;
  std::bitset<HW_EVENT_MAX> registered_hw_events_ = {};
  std::bitset<8> core_id_map_ = 0;
  char path_[64];
#ifdef SEC_GC_QC_VSYNC
  int64_t vsync_registered_timestamp_ = 0;
  int64_t vsync_registered_failed_timestamp_ = 0;
  int vsync_registered_err_code_=0;
#endif
};

}  // namespace sdm

#endif  // __HW_EVENTS_DRM_H__
