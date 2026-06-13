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
#ifndef __SDM_HOTPLUG_H__
#define __SDM_HOTPLUG_H__

#include <atomic>
#include <condition_variable> // NOLINT
#include <mutex>
#include <thread>

#include "sdm_hotplug_cb_intf.h"
#include "sdm_compositor_callbacks.h"

namespace sdm {

class SDMHotPlug {
public:
 explicit SDMHotPlug(SDMHotPlugCbIntf *cb, SDMCompositorCallbacks *callbacks) : cb_(cb) {}
 ~SDMHotPlug() {}

 void Init();
 void Deinit();

private:
  void ParseEvent(char *uevent_data, int length);
  void ListenEvent();
  void ProcessEvent();

  const char *GetTokenValue(const char *uevent_data, int length,
                            const char *token);
  int GetEventValue(const char *uevent_data, int length,
                    const char *event_info);

  std::mutex hpd_mutex_;
  std::condition_variable hpd_cv_;
  std::atomic<int> event_counter_ = 0;
  std::thread hpd_thread_;

  bool hpd_thread_should_terminate_ = false;

  SDMHotPlugCbIntf *cb_ = nullptr;
};

} // namespace sdm

#endif // __SDM_HOTPLUG_H__
