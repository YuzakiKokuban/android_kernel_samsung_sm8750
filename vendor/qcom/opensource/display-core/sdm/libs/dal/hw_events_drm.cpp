/*
* Copyright (c) 2017-2021, The Linux Foundation. All rights reserved.
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

#include <drm_master.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <utils/constants.h>
#include <utils/debug.h>
#include <utils/sys.h>
#include <xf86drm.h>
#include <drm/msm_drm.h>
#include <display/drm/sde_drm.h>

#include <algorithm>
#include <array>
#include <map>
#include <utility>
#include <vector>
#include <string>

#include "hw_events_drm.h"

#ifndef DRM_EVENT_MMRM_CB
#define DRM_EVENT_MMRM_CB 0X8000000B
#endif
#ifndef DRM_EVENT_SDE_HW_RECOVERY
#define DRM_EVENT_SDE_HW_RECOVERY 0x80000007
#endif
#ifndef SDE_RECOVERY_SUCCESS
#define SDE_RECOVERY_SUCCESS 0
#endif
#ifndef SDE_RECOVERY_CAPTURE
#define SDE_RECOVERY_CAPTURE 1
#endif
#ifndef SDE_RECOVERY_DISPLAY_POWER_RESET
#define SDE_RECOVERY_DISPLAY_POWER_RESET 2
#endif
#ifndef DRM_EVENT_VM_RELEASE
#define DRM_EVENT_VM_RELEASE 0X8000000E
#endif
#ifdef SEC_GC_QC_DYN_CLK
#ifndef DRM_EVENT_DYN_CLK
#define DRM_EVENT_DYN_CLK 0X80000020
#endif
#endif

#define __CLASS__ "HWEventsDRM"

namespace sdm {

using drm_utils::DRMMaster;

void HWEventsDRM::HandleDRMOpen(int& fd) {
  for (int i = 0; i < core_id_map_.size(); i++) {
    if (!core_id_map_[i]) {
      continue;
    }

    if (i == 0) {
      fd = drmOpen("msm_drm", nullptr);
      return;
    }

    snprintf(path_, sizeof(path_), "/dev/dri/card%d", i);
    fd = open(path_, O_RDWR | O_CLOEXEC, 0);
    break;
  }
}

DisplayError HWEventsDRM::InitializePollFd() {
  for (uint32_t i = 0; i < event_data_list_.size(); i++) {
    char data[kMaxStringLength]{};
    HWEventData &event_data = event_data_list_[i];
    poll_fds_[i] = {};
    poll_fds_[i].fd = -1;

    switch (event_data.event_type) {
      case HWEvent::VSYNC: {
        poll_fds_[i].events = POLLIN | POLLPRI | POLLERR;
        if (is_primary_) {
          DRMMaster *master = nullptr;
          int ret = DRMMaster::GetInstance(&master);
          if (ret < 0) {
            DLOGE("Failed to acquire DRMMaster instance");
            return kErrorNotSupported;
          }
          master->GetHandle(&poll_fds_[i].fd);
        } else {
          HandleDRMOpen(poll_fds_[i].fd);
        }
        vsync_index_ = i;
      } break;
      case HWEvent::EXIT: {
        // Create an eventfd to be used to unblock the poll system call when
        // a thread is exiting.
        poll_fds_[i].fd = Sys::eventfd_(0, 0);
        poll_fds_[i].events |= POLLIN;
        // Clear any existing data
        Sys::pread_(poll_fds_[i].fd, data, kMaxStringLength, 0);
      } break;
      case HWEvent::IDLE_POWER_COLLAPSE: {
        HandleDRMOpen(poll_fds_[i].fd);
        if (poll_fds_[i].fd < 0) {
          DLOGE("drmOpen failed with error %d", poll_fds_[i].fd);
          return kErrorResources;
        }
        poll_fds_[i].events = POLLIN | POLLPRI | POLLERR;
        idle_pc_index_ = i;
      } break;
      case HWEvent::PANEL_DEAD: {
        HandleDRMOpen(poll_fds_[i].fd);
        if (poll_fds_[i].fd < 0) {
          DLOGE("drmOpen failed with error %d", poll_fds_[i].fd);
          return kErrorResources;
        }
        poll_fds_[i].events = POLLIN | POLLPRI | POLLERR;
        panel_dead_index_ = i;
      } break;
      case HWEvent::HW_RECOVERY: {
        HandleDRMOpen(poll_fds_[i].fd);
        if (poll_fds_[i].fd < 0) {
          DLOGE("drmOpen failed with error %d", poll_fds_[i].fd);
          return kErrorResources;
        }
        poll_fds_[i].events = POLLIN | POLLPRI | POLLERR;
        hw_recovery_index_ = i;
      } break;
      case HWEvent::HISTOGRAM: {
        HandleDRMOpen(poll_fds_[i].fd);
        if (poll_fds_[i].fd < 0) {
          DLOGE("drmOpen failed with error %d", poll_fds_[i].fd);
          return kErrorResources;
        }
        poll_fds_[i].events = POLLIN | POLLPRI | POLLERR;
        histogram_index_ = i;
      } break;
      case HWEvent::BACKLIGHT_EVENT: {
        std::lock_guard<std::mutex> lock(backlight_mutex_);
        int inotify_fd = Sys::inotify_init_();
        if (inotify_fd < 0) {
          DLOGE("inotify init failed");
          return kErrorResources;
        }
        poll_fds_[i].fd = inotify_fd;
        poll_fds_[i].events = POLLIN;
        backlight_event_index_ = i;
        DLOGI("%s backlight_event_index_ %d", brightness_node_.c_str(), backlight_event_index_);
      } break;
      case HWEvent::MMRM: {
        HandleDRMOpen(poll_fds_[i].fd);
        if (poll_fds_[i].fd < 0) {
          DLOGE("drmOpen failed with error %d", poll_fds_[i].fd);
          return kErrorResources;
        }
        poll_fds_[i].events = POLLIN | POLLPRI | POLLERR;
        mmrm_index_ = i;
      } break;
      case HWEvent::POWER_EVENT: {
        HandleDRMOpen(poll_fds_[i].fd);
        if (poll_fds_[i].fd < 0) {
          DLOGE("drmOpen failed with error %d", poll_fds_[i].fd);
          return kErrorResources;
        }
        poll_fds_[i].events = POLLIN | POLLPRI | POLLERR;
        power_event_index_ = i;
      } break;
      case HWEvent::VM_RELEASE_EVENT: {
        HandleDRMOpen(poll_fds_[i].fd);
        if (poll_fds_[i].fd < 0) {
          DLOGE("drmOpen failed with error %d", poll_fds_[i].fd);
          return kErrorResources;
        }
        poll_fds_[i].events = POLLIN | POLLPRI | POLLERR;
        vm_release_event_index_ = i;
      } break;
#ifdef SEC_GC_QC_DYN_CLK
      case HWEvent::DYN_CLK_EVENT: {
        HandleDRMOpen(poll_fds_[i].fd);
        if (poll_fds_[i].fd < 0) {
          DLOGE("drmOpen failed with error %d", poll_fds_[i].fd);
          return kErrorResources;
        }
        poll_fds_[i].events = POLLIN | POLLPRI | POLLERR;
        dynclock_index_ = i;
      } break;
#endif
      default:
        break;
    }
  }

  return kErrorNone;
}

DisplayError HWEventsDRM::SetEventParser() {
  DisplayError error = kErrorNone;

  for (auto &event_data : event_data_list_) {
    switch (event_data.event_type) {
      case HWEvent::VSYNC:
        event_data.event_parser = &HWEventsDRM::HandleVSync;
        break;
      case HWEvent::CEC_READ_MESSAGE:
        event_data.event_parser = &HWEventsDRM::HandleCECMessage;
        break;
      case HWEvent::EXIT:
        event_data.event_parser = &HWEventsDRM::HandleThreadExit;
        break;
      case HWEvent::SHOW_BLANK_EVENT:
        event_data.event_parser = &HWEventsDRM::HandleBlank;
        break;
      case HWEvent::THERMAL_LEVEL:
        event_data.event_parser = &HWEventsDRM::HandleThermal;
        break;
      case HWEvent::IDLE_POWER_COLLAPSE:
        event_data.event_parser = &HWEventsDRM::HandleIdlePowerCollapse;
        break;
      case HWEvent::PANEL_DEAD:
        event_data.event_parser = &HWEventsDRM::HandlePanelDead;
        break;
      case HWEvent::HW_RECOVERY:
        event_data.event_parser = &HWEventsDRM::HandleHwRecovery;
        break;
      case HWEvent::HISTOGRAM:
        event_data.event_parser = &HWEventsDRM::HandleHistogram;
        break;
      case HWEvent::BACKLIGHT_EVENT:
        event_data.event_parser = &HWEventsDRM::HandleBacklightEvent;
        break;
      case HWEvent::MMRM:
        event_data.event_parser = &HWEventsDRM::HandleMMRM;
        break;
      case HWEvent::POWER_EVENT:
        event_data.event_parser = &HWEventsDRM::HandlePowerEvent;
        break;
      case HWEvent::VM_RELEASE_EVENT:
        event_data.event_parser = &HWEventsDRM::HandleVmReleaseEvent;
        break;
#ifdef SEC_GC_QC_DYN_CLK
      case HWEvent::DYN_CLK_EVENT:
        event_data.event_parser = &HWEventsDRM::HandleDynamicClockEvent;
        break;
#endif
      default:
        error = kErrorParameters;
        break;
    }
  }

  return error;
}

void HWEventsDRM::PopulateHWEventData(const vector<HWEvent> &event_list) {
  for (auto &event : event_list) {
    HWEventData event_data;
    event_data.event_type = event;
    event_data_list_.push_back(std::move(event_data));
  }

  SetEventParser();
  InitializePollFd();
}

DisplayError HWEventsDRM::Init(DisplayId display_id, SDMDisplayType display_type,
                               HWEventHandler *event_handler, const vector<HWEvent> &event_list) {
  if (!event_handler)
    return kErrorParameters;

  core_id_map_ = display_id.GetCoreIdMap();
  event_handler->GetDRMDisplayToken(&token_);
  is_primary_ = event_handler->IsPrimaryDisplay();
  std::string backlight_path;
  event_handler->GetPanelBrightnessBasePath(&backlight_path);
  brightness_node_ = backlight_path + "brightness";

  DLOGI("Setup event handler for display %d-%d, CRTC %d, Connector %d", display_id.GetDisplayId(),
        display_type, token_.crtc_id, token_.conn_id);

  event_handler_ = event_handler;
  poll_fds_.resize(event_list.size());

  DLOGI("poll_fd size %d", (int)poll_fds_.size());
  event_thread_name_ += " - " + std::to_string(display_id.GetDisplayId()) + "-" +
                        std::to_string(display_type);

  PopulateHWEventData(event_list);

  if (pthread_create(&event_thread_, NULL, &DisplayEventThread, this) < 0) {
    DLOGE("Failed to start %s, error = %s", event_thread_name_.c_str(), strerror(errno));
    return kErrorResources;
  }

  int value = 0;
  if (Debug::Get()->GetProperty(DISABLE_HW_RECOVERY_PROP, &value) == kErrorNone) {
    disable_hw_recovery_ = (value == 1);
  }
  DLOGI("disable_hw_recovery_ set to %d", disable_hw_recovery_);

  if (Debug::Get()->GetProperty(ENABLE_HISTOGRAM_INTR, &value) == kErrorNone) {
    enable_hist_interrupt_ = (value == 1);
  }
  DLOGI("enable_hist_interrupt_ set to %d", enable_hist_interrupt_);

  if (Debug::Get()->GetProperty(DISABLE_MMRM_PROP, &value) == kErrorNone) {
    disable_mmrm_ = (value == 1);
  }
  DLOGI("disable_mmrm_ set to %d", disable_mmrm_);

  return kErrorNone;
}

DisplayError HWEventsDRM::Deinit() {
  exit_threads_ = true;

  SetEventState(HWEvent::PANEL_DEAD, false);
  SetEventState(HWEvent::IDLE_POWER_COLLAPSE, false);
  SetEventState(HWEvent::HW_RECOVERY, false);
  SetEventState(HWEvent::HISTOGRAM, false);
  SetEventState(HWEvent::MMRM, false);
  SetEventState(HWEvent::POWER_EVENT, false);
  SetEventState(HWEvent::VM_RELEASE_EVENT, false);
#ifdef SEC_GC_QC_DYN_CLK
  SetEventState(HWEvent::DYN_CLK_EVENT, false);
#endif
  Sys::pthread_cancel_(event_thread_);
  WakeUpEventThread();
  pthread_join(event_thread_, NULL);
  CloseFds();

  return kErrorNone;
}

DisplayError HWEventsDRM::SetEventState(HWEvent event, bool enable, void *arg) {
  if (event != HWEvent::VSYNC) {
    if (enable == registered_hw_events_.test(event)) {
      DLOGW("%s of %sregistered hw event %d occurred!!",
            enable ? "Registration" : "Deregistration",
            enable ? "already " : "un-", event);
      return kErrorNone;
    }
  }
  DisplayError error = kErrorNone;
  switch (event) {
    case HWEvent::VSYNC: {
      std::lock_guard<std::mutex> lock(vsync_mutex_);
      vsync_enabled_ = enable;
      if (vsync_enabled_ && !registered_hw_events_.test(HWEvent::VSYNC)) {
        error = RegisterVSync();
        if (error != kErrorNone) {
          return error;
        }
        registered_hw_events_.set(event);
      } else if (!vsync_enabled_) {
        registered_hw_events_.reset(event);
      }
      return kErrorNone;
    }
    case HWEvent::BACKLIGHT_EVENT: {
      std::lock_guard<std::mutex> lock(backlight_mutex_);
      if (backlight_event_index_ == UINT32_MAX) {
        return kErrorResources;
      }
      int inotify_fd = poll_fds_[backlight_event_index_].fd;
      if (!enable) {
        if (backlight_wd_ > 0) {
          Sys::inotify_rm_watch_(inotify_fd, backlight_wd_);
        }
        backlight_wd_ = -1;
      } else if (enable && backlight_wd_ < 0) {
        backlight_wd_ = Sys::inotify_add_watch_(inotify_fd, brightness_node_.c_str(), IN_MODIFY);
        if (backlight_wd_ < 0) {
          DLOGE("inotify_add_watch failed %d", backlight_wd_);
          return kErrorResources;
        }
      }
    } break;
    case HWEvent::POWER_EVENT: {
      if (RegisterPowerEvents(enable) != kErrorNone) {
        return kErrorResources;
      }
    } break;
    case HWEvent::PANEL_DEAD: {
      RegisterPanelDead(enable);
    } break;
    case HWEvent::IDLE_POWER_COLLAPSE: {
      RegisterIdlePowerCollapse(enable);
    } break;
    case HWEvent::HW_RECOVERY: {
      if (!disable_hw_recovery_) {
        RegisterHwRecovery(enable);
      }
    } break;
    case HWEvent::HISTOGRAM: {
      if (enable_hist_interrupt_) {
        RegisterHistogram(enable);
      }
    } break;
    case HWEvent::MMRM: {
      if (!disable_mmrm_) {
        RegisterMMRM(enable);
      }
    } break;
    case HWEvent::VM_RELEASE_EVENT: {
      RegisterVmReleaseEvents(enable);
    } break;
#ifdef SEC_GC_QC_DYN_CLK
	case HWEvent::DYN_CLK_EVENT: {
      RegisterDynamicClockEvent(enable);
    } break;
#endif
    default:
      DLOGE("Event not supported");
      return kErrorNotSupported;
  }
  registered_hw_events_.set(event, enable);

  return kErrorNone;
}

#ifdef SEC_GC_QC_VSYNC
DisplayError HWEventsDRM::GetEventState(bool *enabled, bool *registered, int64_t *timestamp, int64_t *failed_timestamp, int *error) {
  if(!enabled || !registered || !timestamp || !failed_timestamp || !error) {
    return kErrorNotSupported;
  }

  *enabled = vsync_enabled_;
  *registered = registered_hw_events_.test(HWEvent::VSYNC);
  *timestamp = vsync_registered_timestamp_;
  *failed_timestamp = vsync_registered_failed_timestamp_;
  *error = vsync_registered_err_code_;

  return kErrorNone;
}
#endif

void HWEventsDRM::WakeUpEventThread() {
  for (uint32_t i = 0; i < event_data_list_.size(); i++) {
    if (event_data_list_[i].event_type == HWEvent::EXIT && poll_fds_[i].fd >= 0) {
      uint64_t exit_value = 1;
      ssize_t write_size = Sys::write_(poll_fds_[i].fd, &exit_value, sizeof(uint64_t));
      if (write_size != sizeof(uint64_t)) {
        DLOGW("Error triggering exit fd (%d). write size = %zu, error = %s", poll_fds_[i].fd,
              static_cast<size_t>(write_size), strerror(errno));
      }
      break;
    }
  }
}

void HWEventsDRM::CloseFds() {
  for (uint32_t i = 0; i < event_data_list_.size(); i++) {
    switch (event_data_list_[i].event_type) {
      case HWEvent::VSYNC:
        if (!is_primary_) {
          drmClose(poll_fds_[i].fd);
        }
        poll_fds_[i].fd = -1;
        break;
      case HWEvent::EXIT:
        Sys::close_(poll_fds_[i].fd);
        poll_fds_[i].fd = -1;
        break;
      case HWEvent::BACKLIGHT_EVENT: {
        std::lock_guard<std::mutex> lock(backlight_mutex_);
        Sys::inotify_rm_watch_(poll_fds_[i].fd, backlight_wd_);
        Sys::close_(poll_fds_[i].fd);
        poll_fds_[i].fd = -1;
      } break;
      case HWEvent::MMRM:
      case HWEvent::IDLE_POWER_COLLAPSE:
      case HWEvent::PANEL_DEAD:
      case HWEvent::HW_RECOVERY:
      case HWEvent::HISTOGRAM:
      case HWEvent::POWER_EVENT:
      case HWEvent::VM_RELEASE_EVENT:
#ifdef SEC_GC_QC_DYN_CLK
      case HWEvent::DYN_CLK_EVENT:
#endif
        drmClose(poll_fds_[i].fd);
        poll_fds_[i].fd = -1;
        break;
      case HWEvent::CEC_READ_MESSAGE:
      case HWEvent::SHOW_BLANK_EVENT:
      case HWEvent::THERMAL_LEVEL:
      case HWEvent::PINGPONG_TIMEOUT:
        break;
      default:
        break;
    }
  }
}

void *HWEventsDRM::DisplayEventThread(void *context) {
  if (context) {
    return reinterpret_cast<HWEventsDRM *>(context)->DisplayEventHandler();
  }

  return NULL;
}

void *HWEventsDRM::DisplayEventHandler() {
  char data[kMaxStringLength]{};

  prctl(PR_SET_NAME, event_thread_name_.c_str(), 0, 0, 0);
  setpriority(PRIO_PROCESS, 0, kThreadPriorityUrgent);

  // Real Time task with lowest priority.
  struct sched_param param = {0};
  param.sched_priority = sched_get_priority_min(SCHED_FIFO);
  sched_setscheduler(0, SCHED_FIFO, &param);

  while (!exit_threads_) {
    int error = Sys::poll_(poll_fds_.data(), UINT32(poll_fds_.size()), -1);
    if (error <= 0) {
      DLOGW("poll failed. error = %s", strerror(errno));
      continue;
    }

    for (uint32_t i = 0; i < event_data_list_.size(); i++) {
      pollfd &poll_fd = poll_fds_[i];
      if (poll_fd.fd < 0) {
        continue;
      }

      switch (event_data_list_[i].event_type) {
        case HWEvent::VSYNC:
        case HWEvent::PANEL_DEAD:
        case HWEvent::IDLE_POWER_COLLAPSE:
        case HWEvent::HW_RECOVERY:
        case HWEvent::HISTOGRAM:
        case HWEvent::MMRM:
        case HWEvent::POWER_EVENT:
        case HWEvent::VM_RELEASE_EVENT:
#ifdef SEC_GC_QC_DYN_CLK
        case HWEvent::DYN_CLK_EVENT:
#endif
          if (poll_fd.revents & (POLLIN | POLLPRI | POLLERR)) {
            (this->*(event_data_list_[i]).event_parser)(nullptr);
          }
          break;
        case HWEvent::EXIT:
          if ((poll_fd.revents & POLLIN) &&
              (Sys::read_(poll_fd.fd, data, kMaxStringLength) > 0)) {
            (this->*(event_data_list_[i]).event_parser)(data);
          }
          break;
        case HWEvent::BACKLIGHT_EVENT:
          if ((poll_fd.revents & POLLIN)) {
            char buffer[kMaxEventBufferLength] = {};
            int len = 0;
            int length = Sys::read_(poll_fd.fd, buffer, kMaxEventBufferLength);
            while (len < length) {
              struct inotify_event *event = (struct inotify_event *) &buffer[len];
              DLOGI("event masks %x in_modify %x", event->mask, IN_MODIFY);
              if (event->mask & IN_MODIFY) {
                int brightness_fd = Sys::open_(brightness_node_.c_str(), O_RDONLY);
                if (brightness_fd > 0) {
                  if (Sys::read_(brightness_fd, data, kMaxStringLength) > 0) {
                      (this->*(event_data_list_[i]).event_parser)(data);
                  }
                  Sys::close_(brightness_fd);
                }
              }
              len += sizeof(struct inotify_event) + event->len;
            }
          }  break;
        case HWEvent::CEC_READ_MESSAGE:
        case HWEvent::SHOW_BLANK_EVENT:
        case HWEvent::THERMAL_LEVEL:
        case HWEvent::PINGPONG_TIMEOUT:
          if ((poll_fd.revents & POLLPRI) &&
              (Sys::pread_(poll_fd.fd, data, kMaxStringLength, 0) > 0)) {
            (this->*(event_data_list_[i]).event_parser)(data);
          }
          break;
        default:
          break;
      }
    }
  }

  DLOGI("Exiting the thread");
  pthread_exit(0);

  return nullptr;
}

DisplayError HWEventsDRM::RegisterVSync() {
  DTRACE_SCOPED();
  drmVBlank vblank {};
  uint32_t high_crtc = token_.crtc_index << DRM_VBLANK_HIGH_CRTC_SHIFT;
  vblank.request.type = (drmVBlankSeqType)(DRM_VBLANK_RELATIVE | DRM_VBLANK_EVENT |
                                           (high_crtc & DRM_VBLANK_HIGH_CRTC_MASK));
  vblank.request.sequence = 1;
  // DRM hack to pass in context to unused field signal. Driver will write this to the node being
  // polled on, and will be read as part of drm event handling and sent to handler
  vblank.request.signal = reinterpret_cast<unsigned long>(this);  // NOLINT
#ifdef SEC_GC_QC_VSYNC
  struct timespec ts;
  if(clock_gettime(CLOCK_MONOTONIC, &ts) != EINVAL) {
    vsync_registered_timestamp_ = (int64_t)(ts.tv_sec)*1000000000 + (int64_t)(ts.tv_nsec);
  }
#endif
  int error = drmWaitVBlank(poll_fds_[vsync_index_].fd, &vblank);
  if (error < 0) {
    DLOGE("drmWaitVBlank failed with err %d", errno);
#ifdef SEC_GC_QC_VSYNC
    if(clock_gettime(CLOCK_MONOTONIC, &ts) != EINVAL) {
      vsync_registered_failed_timestamp_ = (int64_t)(ts.tv_sec)*1000000000 + (int64_t)(ts.tv_nsec);
    }
    vsync_registered_err_code_ = errno;
#endif
    return kErrorResources;
  }

  return kErrorNone;
}

DisplayError HWEventsDRM::RegisterPanelDead(bool enable) {
  if (panel_dead_index_ == UINT32_MAX) {
    DLOGI("panel dead is not supported event");
    return kErrorNone;
  }

  struct drm_msm_event_req req = {};
  int ret = 0;

  req.object_id = token_.conn_id;
  req.object_type = DRM_MODE_OBJECT_CONNECTOR;
  req.event = DRM_EVENT_PANEL_DEAD;
  if (enable) {
    ret = drmIoctl(poll_fds_[panel_dead_index_].fd, DRM_IOCTL_MSM_REGISTER_EVENT, &req);
  } else {
    ret = drmIoctl(poll_fds_[panel_dead_index_].fd, DRM_IOCTL_MSM_DEREGISTER_EVENT, &req);
  }

  if (ret) {
    DLOGE("register panel dead enable:%d failed", enable);
    return kErrorResources;
  }

  return kErrorNone;
}

DisplayError HWEventsDRM::RegisterPowerEvents(bool enable) {
  if (power_event_index_ == UINT32_MAX) {
    DLOGI("power event is not supported");
    return kErrorNone;
  }

  struct drm_msm_event_req req = {};
  int ret = 0;

  req.object_id = token_.crtc_id;
  req.object_type = DRM_MODE_OBJECT_CRTC;
  req.event = DRM_EVENT_CRTC_POWER;

  if (enable) {
    ret = drmIoctl(poll_fds_[power_event_index_].fd, DRM_IOCTL_MSM_REGISTER_EVENT, &req);
  } else {
    ret = drmIoctl(poll_fds_[power_event_index_].fd, DRM_IOCTL_MSM_DEREGISTER_EVENT, &req);
  }
  if (ret) {
    ret = -errno;
    if (ret == -ENOENT || ret == -ENODEV || ret == -EACCES) {
      DLOGW("%s event failed as the device has disconnected. Event_thread_name : %s Ret=%d",
            (enable) ? "Register" : "DeRegister", event_thread_name_.c_str(), ret);
    } else {
      DLOGE("Failed to %s event. Event_thread_name : %s, Ret=%d", (enable) ? "Register" :
            "DeRegister", event_thread_name_.c_str(), ret);
    }
    return kErrorResources;
  }
  return kErrorNone;
}

DisplayError HWEventsDRM::RegisterHistogram(bool enable) {
  if (histogram_index_ == UINT32_MAX) {
    DLOGI("histogram is not supported event");
    return kErrorNone;
  }
  struct drm_msm_event_req req = {};
  int ret = 0;

  req.object_id = token_.crtc_id;
  req.object_type = DRM_MODE_OBJECT_CRTC;
  req.event = DRM_EVENT_HISTOGRAM;
  if (enable) {
    ret = drmIoctl(poll_fds_[histogram_index_].fd, DRM_IOCTL_MSM_REGISTER_EVENT, &req);
  } else {
    ret = drmIoctl(poll_fds_[histogram_index_].fd, DRM_IOCTL_MSM_DEREGISTER_EVENT, &req);
  }

  if (ret) {
    DLOGE("register histogram enable:%d failed", enable);
    return kErrorResources;
  }

  return kErrorNone;
}

DisplayError HWEventsDRM::RegisterIdlePowerCollapse(bool enable) {
  if (idle_pc_index_ == UINT32_MAX) {
    DLOGI("idle power collapse is not supported event");
    return kErrorNone;
  }

  struct drm_msm_event_req req = {};
  int ret = 0;

  req.object_id = token_.crtc_id;
  req.object_type = DRM_MODE_OBJECT_CRTC;
  req.event = DRM_EVENT_SDE_POWER;
  if (enable) {
    ret = drmIoctl(poll_fds_[idle_pc_index_].fd, DRM_IOCTL_MSM_REGISTER_EVENT, &req);
  } else {
    ret = drmIoctl(poll_fds_[idle_pc_index_].fd, DRM_IOCTL_MSM_DEREGISTER_EVENT, &req);
  }

  if (ret) {
    DLOGE("register idle power collapse enable:%d failed", enable);
    return kErrorResources;
  }

  return kErrorNone;
}

DisplayError HWEventsDRM::RegisterHwRecovery(bool enable) {
  if (hw_recovery_index_ == UINT32_MAX) {
    DLOGI("Hardware recovery is not supported");
    return kErrorNone;
  }

  struct drm_msm_event_req req = {};
  int ret = 0;

  req.object_id = token_.conn_id;
  req.object_type = DRM_MODE_OBJECT_CONNECTOR;
  req.event = DRM_EVENT_SDE_HW_RECOVERY;
  if (enable) {
    ret = drmIoctl(poll_fds_[hw_recovery_index_].fd, DRM_IOCTL_MSM_REGISTER_EVENT, &req);
  } else {
    ret = drmIoctl(poll_fds_[hw_recovery_index_].fd, DRM_IOCTL_MSM_DEREGISTER_EVENT, &req);
  }
  if (ret) {
    DLOGE("Register hardware recovery enable:%d failed", enable);
    return kErrorResources;
  }

  DLOGI("Register hw recovery %s", enable ? "enable" : "disable");
  return kErrorNone;
}

DisplayError HWEventsDRM::RegisterMMRM(bool enable) {
  if (mmrm_index_ == UINT32_MAX) {
    DLOGI("MMRM is not supported");
    return kErrorNone;
  }

  struct drm_msm_event_req req = {};
  int ret = 0;

  req.object_id = token_.crtc_id;
  req.object_type = DRM_MODE_OBJECT_CRTC;
  req.event = DRM_EVENT_MMRM_CB;
  if (enable) {
    ret = drmIoctl(poll_fds_[mmrm_index_].fd, DRM_IOCTL_MSM_REGISTER_EVENT, &req);
  } else {
    ret = drmIoctl(poll_fds_[mmrm_index_].fd, DRM_IOCTL_MSM_DEREGISTER_EVENT, &req);
  }
  if (ret) {
    DLOGE("Register MMRM enable:%d failed", enable);
    return kErrorResources;
  }

  return kErrorNone;
}

DisplayError HWEventsDRM::RegisterVmReleaseEvents(bool enable) {
  if (vm_release_event_index_ == UINT32_MAX) {
    DLOGI("Vm Release is not supported event");
    return kErrorNone;
  }
  struct drm_msm_event_req req = {};
  int ret = 0;

  req.object_id = token_.crtc_id;
  req.object_type = DRM_MODE_OBJECT_CRTC;
  req.event = DRM_EVENT_VM_RELEASE;
  if (enable) {
    ret = drmIoctl(poll_fds_[vm_release_event_index_].fd, DRM_IOCTL_MSM_REGISTER_EVENT, &req);
  } else {
    ret = drmIoctl(poll_fds_[vm_release_event_index_].fd, DRM_IOCTL_MSM_DEREGISTER_EVENT, &req);
  }

  if (ret) {
    DLOGE("register vm release event %s failed with ret %d", enable ? "enable" : "disable", ret);
    return kErrorResources;
  }

  DLOGI("Register vm release event %s successful", enable ? "enable" : "disable");
  return kErrorNone;
}

#ifdef SEC_GC_QC_DYN_CLK
DisplayError HWEventsDRM::RegisterDynamicClockEvent(bool enable) {
  if (dynclock_index_ == UINT32_MAX) {
    DLOGI("histogram is not supported event");
    return kErrorNone;
  }
  struct drm_msm_event_req req = {};
  int ret = 0;

  req.object_id = token_.conn_id;
  req.object_type = DRM_MODE_OBJECT_CONNECTOR;
  req.event = DRM_EVENT_DYN_CLK;
  if (enable) {
    ret = drmIoctl(poll_fds_[dynclock_index_].fd, DRM_IOCTL_MSM_REGISTER_EVENT, &req);
  } else {
    ret = drmIoctl(poll_fds_[dynclock_index_].fd, DRM_IOCTL_MSM_DEREGISTER_EVENT, &req);
  }

  if (ret) {
    DLOGE("DYN_CLK_EVENT register dynamic clock enable:%d failed:%d index:%d", enable, ret, dynclock_index_);
    return kErrorResources;
  }

  return kErrorNone;
}
#endif

void HWEventsDRM::HandleVSync(char *data) {
  DisplayError ret = kErrorNone;
  vsync_handler_count_ = 0;  //  reset vsync handler count. lock not needed
  {
    std::lock_guard<std::mutex> lock(vsync_mutex_);
    registered_hw_events_.reset(HWEvent::VSYNC);
    if (vsync_enabled_) {
      ret = RegisterVSync();
      if (ret == kErrorNone)
        registered_hw_events_.set(HWEvent::VSYNC);
    }
  }

  drmEventContext event = {};
  event.version = DRM_EVENT_CONTEXT_VERSION;
  event.vblank_handler = &HWEventsDRM::VSyncHandlerCallback;
  int error = drmHandleEvent(poll_fds_[vsync_index_].fd, &event);
  if (error != 0) {
    DLOGE("drmHandleEvent failed: %i", error);
  }

  if (vsync_handler_count_ > 1) {
    //  probable thread preemption caused > 1 vsync handling. Re-enable vsync before polling
    std::lock_guard<std::mutex> lock(vsync_mutex_);
    registered_hw_events_.reset(HWEvent::VSYNC);
    if (vsync_enabled_) {
      ret = RegisterVSync();
      if (ret == kErrorNone)
        registered_hw_events_.set(HWEvent::VSYNC);
    }
  }
}

void HWEventsDRM::HandlePanelDead(char *data) {
  char event_data[kMaxStringLength] = {0};
  int32_t size;
  struct drm_msm_event_resp *event_resp = NULL;

  size = (int32_t)Sys::pread_(poll_fds_[panel_dead_index_].fd, event_data, kMaxStringLength, 0);
  if (size <= 0) {
    return;
  }

  if (size > kMaxStringLength) {
    DLOGE("event size %d is greater than event buffer size %d\n", size, kMaxStringLength);
    return;
  }

  if (size < (int32_t)sizeof(*event_resp)) {
    DLOGE("Invalid event size %d expected %zd\n", size, sizeof(*event_resp));
    return;
  }

  int32_t i = 0;
  while (i < size) {
    event_resp = (struct drm_msm_event_resp *)&event_data[i];
    switch (event_resp->base.type) {
      case DRM_EVENT_PANEL_DEAD:
      {
        DLOGI("Received panel dead event");
        event_handler_->PanelDead();
        break;
      }
      default: {
        DLOGE("invalid event %d", event_resp->base.type);
        break;
      }
    }
    i += event_resp->base.length;
  }

  return;
}

void HWEventsDRM::VSyncHandlerCallback(int fd, unsigned int sequence, unsigned int tv_sec,
                                       unsigned int tv_usec, void *data) {
  HWEventsDRM *ev_data = reinterpret_cast<HWEventsDRM *>(data);
  ev_data->vsync_handler_count_++;
  int64_t timestamp = (int64_t)(tv_sec)*1000000000 + (int64_t)(tv_usec)*1000;
  DTRACE_SCOPED();
  ev_data->event_handler_->VSync(timestamp);
}

void HWEventsDRM::HandleCECMessage(char *data) {
  event_handler_->CECMessage(data);
}

void HWEventsDRM::HandleIdlePowerCollapse(char *data) {
  char event_data[kMaxStringLength];
  int32_t size;
  struct drm_msm_event_resp *event_resp = NULL;

  size = (int32_t)Sys::pread_(poll_fds_[idle_pc_index_].fd, event_data, kMaxStringLength, 0);
  if (size < 0) {
    return;
  }

  if (size > kMaxStringLength) {
    DLOGE("event size %d is greater than event buffer size %d\n", size, kMaxStringLength);
    return;
  }

  if (size < (int32_t)sizeof(*event_resp)) {
    DLOGE("size %d exp %zd\n", size, sizeof(*event_resp));
    return;
  }

  int32_t i = 0;

  while (i < size) {
    event_resp = (struct drm_msm_event_resp *)&event_data[i];
    switch (event_resp->base.type) {
      case DRM_EVENT_SDE_POWER:
      {
        uint32_t* event_payload = reinterpret_cast<uint32_t *>(event_resp->data);
        if (*event_payload == 0) {
          DLOGV("Received Idle power collapse event");
          event_handler_->IdlePowerCollapse();
        }
        break;
      }
      default: {
        DLOGE("invalid event %d", event_resp->base.type);
        break;
      }
    }
    i += event_resp->base.length;
  }

  return;
}

void HWEventsDRM::HandleHwRecovery(char *data) {
  char event_data[kMaxStringLength] = {0};
  int32_t size;
  struct drm_msm_event_resp *event_resp = NULL;

  size = (int32_t)Sys::pread_(poll_fds_[hw_recovery_index_].fd, event_data, kMaxStringLength, 0);
  if (size < 0) {
    return;
  }

  if (size > kMaxStringLength) {
    DLOGE("Hardware recovery event size %d is greater than event buffer size %d\n", size,
          kMaxStringLength);
    return;
  }

  if (size < (int32_t)sizeof(*event_resp)) {
    DLOGE("Hardware recovery event size is %d, expected %zd\n", size, sizeof(*event_resp));
    return;
  }

  int32_t i = 0;

  while (i < size) {
    event_resp = (struct drm_msm_event_resp *)&event_data[i];
    switch (event_resp->base.type) {
      case DRM_EVENT_SDE_HW_RECOVERY: {
        std::size_t size_of_data = (std::size_t)event_resp->base.length -
                                   (sizeof(event_resp->base) + sizeof(event_resp->info));
        // expect up to uint32_t from driver
        if (size_of_data > sizeof(uint32_t)) {
          DLOGE("Size of hardware recovery event data: %zu exceeds %zu", size_of_data,
                sizeof(uint32_t));
          return;
        }

        uint32_t hw_event_code = 0;
        memcpy(&hw_event_code, event_resp->data, size_of_data);

        HWRecoveryEvent sdm_event_code;
        if (SetHwRecoveryEvent(hw_event_code, &sdm_event_code)) {
          return;
        }
        event_handler_->HwRecovery(sdm_event_code);
        break;
      }
      default: {
        DLOGE("Invalid event type %d", event_resp->base.type);
        break;
      }
    }
    i += event_resp->base.length;
  }

  return;
}

void HWEventsDRM::HandlePowerEvent(char * /*data*/) {
  DTRACE_SCOPED();
  auto constexpr expected_size = sizeof(drm_msm_event_resp) + sizeof(uint32_t);
  std::array<char, expected_size> event_data{'\0'};
  auto size = Sys::pread_(poll_fds_[power_event_index_].fd, event_data.data(),
                          event_data.size(), 0);
  if (size != expected_size) {
    DLOGE("event size %d is unexpected. skipping this power event", UINT32(size));
    return;
  }

  auto msm_event = reinterpret_cast<struct drm_msm_event_resp *>(event_data.data());
  DLOGI("poweron %d", *(reinterpret_cast<uint32_t *>(msm_event->data)));

  event_handler_->HandlePowerEvent();
}

void HWEventsDRM::HandleHistogram(char * /*data*/) {
  auto constexpr expected_size = sizeof(drm_msm_event_resp) + sizeof(uint32_t);
  std::array<char, expected_size> event_data{'\0'};
  auto size = Sys::pread_(poll_fds_[histogram_index_].fd, event_data.data(), event_data.size(), 0);
  if (size != expected_size) {
    DLOGE("event size %d is unexpected. skipping this histogram event", UINT32(size));
    return;
  }

  auto msm_event = reinterpret_cast<struct drm_msm_event_resp *>(event_data.data());
  auto blob_id = reinterpret_cast<uint32_t *>(msm_event->data);
  event_handler_->Histogram(poll_fds_[histogram_index_].fd, *blob_id);
}

void HWEventsDRM::HandleBacklightEvent(char *data) {
  event_handler_->HandleBacklightEvent(atof(data));
}

void HWEventsDRM::HandleMMRM(char *data) {
  DTRACE_SCOPED();
  char event_data[kMaxStringLength];
  int32_t size;
  struct drm_msm_event_resp *event_resp = NULL;

  size = (int32_t)Sys::pread_(poll_fds_[mmrm_index_].fd, event_data, kMaxStringLength, 0);
  if (size < 0) {
    DLOGW("Size is invalid!");
    return;
  }

  if (size > kMaxStringLength) {
    DLOGE("event size %d is greater than event buffer size %d\n", size, kMaxStringLength);
    return;
  }

  if (size < (int32_t)sizeof(*event_resp)) {
    DLOGE("size %d exp %zd\n", size, sizeof(*event_resp));
    return;
  }

  int32_t i = 0;

  while (i < size) {
    event_resp = (struct drm_msm_event_resp *)&event_data[i];
    switch (event_resp->base.type) {
      case DRM_EVENT_MMRM_CB:
      {
        uint32_t* event_payload = reinterpret_cast<uint32_t *>(event_resp->data);
        if (event_payload) {
          DLOGV("Received MMRM event");
          event_handler_->MMRMEvent(*event_payload);
        }
        break;
      }
      default: {
        DLOGE("invalid event %d", event_resp->base.type);
        break;
      }
    }
    i += event_resp->base.length;
  }

  return;
}

int HWEventsDRM::SetHwRecoveryEvent(const uint32_t hw_event_code, HWRecoveryEvent *sdm_event_code) {
  switch (hw_event_code) {
     case SDE_RECOVERY_SUCCESS:
       *sdm_event_code = HWRecoveryEvent::kSuccess;
       break;
     case SDE_RECOVERY_CAPTURE:
       *sdm_event_code = HWRecoveryEvent::kCapture;
       break;
     case SDE_RECOVERY_DISPLAY_POWER_RESET:
       *sdm_event_code = HWRecoveryEvent::kDisplayPowerReset;
       break;
     default:
       DLOGE("Unsupported hardware recovery event value received = %" PRIu32, hw_event_code);
       return -EINVAL;
  }

  return 0;
}

void HWEventsDRM::HandleVmReleaseEvent(char * /*data*/) {
  auto constexpr expected_size = sizeof(drm_msm_event_resp) + sizeof(uint32_t);
  std::array<char, expected_size> event_data{'\0'};
  auto size = Sys::pread_(poll_fds_[vm_release_event_index_].fd, event_data.data(),
                          event_data.size(), 0);
  if (size != expected_size) {
    DLOGE("event size %d is unexpected. skipping this vm release event", UINT32(size));
    return;
  }

  auto msm_event = reinterpret_cast<struct drm_msm_event_resp *>(event_data.data());
  DLOGI("vm release event data %d", *(reinterpret_cast<uint32_t *>(msm_event->data)));
  event_handler_->HandleVmReleaseEvent();
}

#ifdef SEC_GC_QC_DYN_CLK
void HWEventsDRM::HandleDynamicClockEvent(char * /*data*/) {
	  auto constexpr expected_size = sizeof(drm_msm_event_resp) + sizeof(uint32_t);
	  std::array<char, expected_size> event_data{'\0'};
	  auto size = Sys::pread_(poll_fds_[dynclock_index_].fd, event_data.data(), event_data.size(), 0);
	  if (size != expected_size) {
		DLOGE("event size %d is unexpected. skipping this dynamic dsi clock change event", size);
		return;
	  }
	
	  auto msm_event = reinterpret_cast<struct drm_msm_event_resp *>(event_data.data());
	  auto dsi_clk = (reinterpret_cast<uint32_t *>(msm_event->data));

	  DLOGI("[DynamicClock] Received dynamic dsi clock change event, %x (%d)", *dsi_clk, *dsi_clk);

	  event_handler_->SetDynamicDSIClockCustom(*dsi_clk);
}
#endif
}  // namespace sdm
