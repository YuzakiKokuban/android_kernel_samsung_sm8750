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
#include <utils/debug.h>

#include "sdm_cwb.h"

#include <MetadataType.h>

#define __CLASS__ "SDMConcurrentWriteBack"

namespace sdm {

using MetadataType = vendor_qti_hardware_display_common_MetadataType;

DisplayError SDMConcurrentWriteBack::PostBuffer(const CwbConfig &cwb_config,
                                                void *hdl, int dpy_index) {
  DisplayError error = kErrorNone;
  auto &session_map = display_cwb_session_map_[dpy_index];
  std::shared_ptr<QueueNode> node = nullptr;
  uint64_t node_handle_id = 0;
  SnapHandle *handle = reinterpret_cast<SnapHandle *>(hdl);
  auto snap_err = snapmapper_->GetMetadata(*handle, MetadataType::BUFFER_ID, &node_handle_id);
  if (snap_err != Error::NONE || node_handle_id == 0) {
    error = kErrorParameters;
    DLOGE("Buffer handle id retrieval failed!");
  }

  if (error == kErrorNone) {
    node =
        std::make_shared<QueueNode>(cwb_config, hdl, dpy_index, node_handle_id);
    if (node) {
      // Keep CWB request handling related resources in a requested display
      // context.
      std::unique_lock<std::mutex> lock(session_map.lock);

      // Iterate over the queue to avoid duplicate node of same buffer, because
      // that buffer is already present in queue.
      for (auto &qnode : session_map.queue) {
        if (qnode->handle_id == node_handle_id) {
          error = kErrorParameters;
          DLOGW("CWB Buffer with handle id %lu is already available in Queue "
                "for processing!",
                node_handle_id);
          break;
        }
      }

      // Ensure that async task runs only until all queued CWB requests have
      // been fulfilled. If cwb queue is empty, async task has not either
      // started or async task has finished processing previously queued cwb
      // requests. Start new async task on such a case as currently running
      // async task will automatically desolve without processing more requests.
      if (error == kErrorNone) {
        session_map.queue.push_back(node);
      }
    } else {
      error = kErrorParameters;
      DLOGE("Unable to allocate node for CWB request(handle id: %lu)!",
            node_handle_id);
    }
  }

  if (error == kErrorNone) {
    int cb_err = cb_->HandleCwbCallBack(dpy_index, hdl, cwb_config);
    if (cb_err) {
      error = kErrorParameters;
    }
  }

  if (error == kErrorNone) {
    DLOGV_IF(kTagCwb, "Successfully configured CWB buffer(handle id: %lu).",
             node_handle_id);
  } else {
    std::unique_lock<std::mutex> lock(session_map.lock);
    // If current node is pushed in the queue, then need to remove it again on
    // error.
    if (node && node == session_map.queue.back()) {
      session_map.queue.pop_back();
    }
    return kErrorNotSupported;
  }

  std::unique_lock<std::mutex> lock(session_map.lock);
  if (!session_map.async_thread_running && !session_map.queue.empty()) {
    session_map.async_thread_running = true;
    // No need to do future.get() here for previously running async task. Async
    // method will guarantee to exit after cwb for all queued requests is indeed
    // complete i.e. the respective fences have signaled and client is notified
    // through registered callbacks. This will make sure that the new async task
    // does not concurrently work with previous task. Let async running thread
    // dissolve on its own. Check, If thread is not running, then need to
    // re-execute the async thread.
    session_map.future = std::async(
        SDMConcurrentWriteBack::AsyncTaskToProcessCWBStatus, this, dpy_index);
  }

  if (node) {
    node->request_completed = true;
  }

  return kErrorNone;
}

DisplayError SDMConcurrentWriteBack::OnCWBDone(int dpy_index, int32_t status,
                                               uint64_t handle_id) {
  auto &session_map = display_cwb_session_map_[dpy_index];

  {
    std::unique_lock<std::mutex> lock(session_map.lock);
    // No need to notify to the client, if there is no any pending CWB request
    // in queue.
    if (session_map.queue.empty()) {
      return kErrorNotSupported;
    }

    for (auto &node : session_map.queue) {
      if (node->notified_status == kCwbNotifiedNone) {
        // Need to wait for other notification, when notified handle id does not
        // match with available first non-notified node buffer handle id in
        // queue.
        if (node->handle_id == handle_id) {
          node->notified_status =
              (status) ? kCwbNotifiedFailure : kCwbNotifiedSuccess;
          session_map.cv.notify_one();
          return kErrorNone;
        } else {
          // Continue to check on not matching handle_id, to update the status
          // of any matching node, because if notification for particular
          // handle_id skip, then it will not update again and notification
          // thread will wait for skipped node forever.
          continue;
        }
      }
    }
  }

  return kErrorNotSupported;
}

void SDMConcurrentWriteBack::AsyncTaskToProcessCWBStatus(
    SDMConcurrentWriteBack *cwb, int dpy_index) {
  cwb->ProcessCWBStatus(dpy_index);
}

void SDMConcurrentWriteBack::ProcessCWBStatus(int dpy_index) {
  auto &session_map = display_cwb_session_map_[dpy_index];
  while (true) {
    std::shared_ptr<QueueNode> cwb_node = nullptr;
    {
      std::unique_lock<std::mutex> lock(session_map.lock);
      // Exit thread in case of no pending CWB request in queue.
      if (session_map.queue.empty()) {
        // Update thread exiting status.
        session_map.async_thread_running = false;
        break;
      }

      cwb_node = session_map.queue.front();
      if (!cwb_node->request_completed) {
        // Need to continue to recheck until node specific client call
        // completes.
        continue;
      } else if (cwb_node->notified_status == kCwbNotifiedNone) {
        // Wait for the signal for availability of CWB notified node.
        session_map.cv.wait(lock);
        if (cwb_node->notified_status == kCwbNotifiedNone) {
          // If any other node notified before front node, then need to continue
          // to wait for front node, such that further client notification will
          // be done in sequential manner.
          DLOGW("CWB request is notified out of sequence.");
          continue;
        }
      }
      session_map.queue.pop_front();
    }

    // Notify to client, when notification is received successfully for expected
    // input buffer.
    cb_->NotifyCWBStatus(cwb_node->notified_status, cwb_node->buffer);
  }
  DLOGI("CWB queue is empty. Display: %d", dpy_index);
}

} // namespace sdm
