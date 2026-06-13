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
#include <sys/prctl.h>
#include <sys/resource.h>
#include <utils/constants.h>
#include <utils/debug.h>
#include <utils/utils.h>
#include <unistd.h>
#include <pthread.h>

#include "sdm_hotplug.h"
#include "sdm_common.h"

#define __CLASS__ "SDMHotPlug"
#define SDM_UEVENT_DRM_EXT_HOTPLUG "mdss_mdp/drm/card"

namespace sdm {

void SDMHotPlug::Init() {
  hpd_thread_ = std::thread(&SDMHotPlug::ProcessEvent, this);
  std::thread(&SDMHotPlug::ListenEvent, this).detach();
}

void SDMHotPlug::Deinit() {
  if (hpd_thread_.joinable()) {
    hpd_thread_should_terminate_ = true;
    hpd_cv_.notify_one();
    hpd_thread_.join();
  }
}

const char *SDMHotPlug::GetTokenValue(const char *uevent_data, int length,
                                      const char *token) {
  const char *iterator_str = uevent_data;
  const char *pstr = NULL;
  while (((iterator_str - uevent_data) <= length) && (*iterator_str)) {
    pstr = strstr(iterator_str, token);
    if (pstr) {
      break;
    }
    iterator_str += strlen(iterator_str) + 1;
  }

  if (pstr)
    pstr = pstr + strlen(token);

  return pstr;
}

int SDMHotPlug::GetEventValue(const char *uevent_data, int length,
                              const char *event_info) {
  const char *iterator_str = uevent_data;
  while (((iterator_str - uevent_data) <= length) && (*iterator_str)) {
    const char *pstr = strstr(iterator_str, event_info);
    if (pstr != NULL) {
      return (atoi(iterator_str + strlen(event_info)));
    }
    iterator_str += strlen(iterator_str) + 1;
  }

  return -1;
}

void SDMHotPlug::ParseEvent(char *data, int length) {
  static constexpr uint32_t uevent_max_count = 3;
  const char *str_status = GetTokenValue(data, length, "status=");
  const char *str_sstmst = GetTokenValue(data, length, "HOTPLUG=");
  const char *str_mst = GetTokenValue(data, length, "MST_HOTPLUG=");

  if (!str_status && !str_mst && !str_sstmst) {
    return;
  }

  if (!strcasestr(data, SDM_UEVENT_DRM_EXT_HOTPLUG)) {
    return;
  }

  int hpd_bpp = GetEventValue(data, length, "bpp=");
  int hpd_pattern = GetEventValue(data, length, "pattern=");

  DLOGI("UEvent = %s, status = %s, HOTPLUG = %s (SST/MST)%s%s, bpp = %d, "
        "pattern = %d",
        data, str_status ? str_status : "NULL",
        str_sstmst ? str_sstmst : "NULL", str_mst ? ", MST_HOTPLUG = " : "",
        str_mst ? str_mst : "", hpd_bpp, hpd_pattern);

  int hpd_connected = 0;
  if (str_status) {
    hpd_connected = strncmp(str_status, "connected", strlen("connected")) == 0;
    DLOGI("Connected = %d", hpd_connected);
  }

  cb_->SetHpdData(hpd_bpp, hpd_pattern, hpd_connected);

  event_counter_++;
  std::unique_lock<std::mutex> evt_lock(hpd_mutex_);
  if (event_counter_.load() > uevent_max_count) {
    event_counter_.store(uevent_max_count);
  }

  hpd_cv_.notify_one();
}

void SDMHotPlug::ListenEvent() {
  DLOGI("Starting!");
  const char *uevent_thread_name = "sdm_hpd_listen_event";

  prctl(PR_SET_NAME, uevent_thread_name, 0, 0, 0);
  setpriority(PRIO_PROCESS, 0, 0);

  int status = uevent_init();
  if (!status) {
    DLOGE("Failed to init uevent with err %d", status);
    return;
  }

  while (1) {
    char uevent_data[get_page_size()] = {};

    // keep last 2 zeros to ensure double 0 termination
    int length = uevent_next_event(uevent_data, INT32(sizeof(uevent_data)) - 2);

    ParseEvent(uevent_data, length);
  }
  DLOGI("Ending!");
}

void SDMHotPlug::ProcessEvent() {
  DLOGI("Starting!");
  const char *uevent_thread_name = "sdm_hpd_process_event";

  prctl(PR_SET_NAME, uevent_thread_name, 0, 0, 0);
  setpriority(PRIO_PROCESS, 0, 0);

  std::unique_lock<std::mutex> evt_lock(hpd_mutex_);
  while (1) {
    hpd_cv_.wait(evt_lock);

    if (hpd_thread_should_terminate_) {
      break;
    }

    while (event_counter_.load() > 0) {
      evt_lock.unlock();
      cb_->HpdEventHandler();
      evt_lock.lock();

      event_counter_--;
    }
  }
  DLOGI("Ending!");
}

} // namespace sdm
