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
#ifndef __SDM_CWB_H__
#define __SDM_CWB_H__

#include <core/layer_buffer.h>

#include <atomic>
#include <condition_variable> // NOLINT
#include <deque>
#include <future> // NOLINT
#include <map>
#include <thread>

#include "sdm_cwb_cb_intf.h"
#include "sdm_display_intf_parcel.h"

#include <ISnapMapper.h>
#include <SnapHandle.h>

namespace sdm {

using vendor::qti::hardware::display::snapalloc::ISnapMapper;
using vendor::qti::hardware::display::snapalloc::Error;
using vendor::qti::hardware::display::snapalloc::SnapHandle;

class SDMConcurrentWriteBack {
public:
  explicit SDMConcurrentWriteBack(SDMConcurrentWriteBackCbIntf *cb,
            std::shared_ptr<ISnapMapper> mapper) : cb_(cb), snapmapper_(mapper) {}
  void Init() {}

  DisplayError PostBuffer(const CwbConfig &cwb_config, void *buffer,
                          int32_t display_type);
  DisplayError OnCWBDone(int dpy_index, int32_t status, uint64_t handle_id);

private:
  enum CWBNotifiedStatus {
    kCwbNotifiedFailure = -1,
    kCwbNotifiedSuccess,
    kCwbNotifiedNone,
  };

  struct QueueNode {
    QueueNode(const CwbConfig &cwb_conf, void *buf, Display disp_type,
              uint64_t buf_id)
        : cwb_config(cwb_conf), buffer(buf), display_type(disp_type),
          handle_id(buf_id) {}

    CwbConfig cwb_config = {};
    void *buffer;
    Display display_type;
    uint64_t handle_id;
    CWBNotifiedStatus notified_status = kCwbNotifiedNone;
    bool request_completed = false;
  };

  struct DisplayCWBSession {
    std::deque<std::shared_ptr<QueueNode>> queue;
    std::mutex lock;
    std::condition_variable cv;
    std::future<void> future;
    bool async_thread_running = false;
  };

  static void AsyncTaskToProcessCWBStatus(SDMConcurrentWriteBack *cwb,
                                          int dpy_index);
  void ProcessCWBStatus(int dpy_index);

  std::map<int, DisplayCWBSession> display_cwb_session_map_;
  SDMConcurrentWriteBackCbIntf *cb_ = nullptr;
  std::shared_ptr<ISnapMapper> snapmapper_ = nullptr;
};

} // namespace sdm

#endif // __SDM_CWB_H__
