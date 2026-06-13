/*
 * Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *
 *    * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 * GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 * HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
* Changes from Qualcomm Innovation Center are provided under the following license:
*
* Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#ifndef __CWB_MANAGER_INTERFACE_H__
#define __CWB_MANAGER_INTERFACE_H__

#include <core/layer_buffer.h>

#include <utils/sys.h>
#include <utils/debug.h>

#include "hw_info_types.h"

namespace sdm {

class CwbManagerInterface;

extern "C" CwbManagerInterface* GetCwbManagerInterface();

class CwbCallback {
 public:
  virtual ~CwbCallback() {}
  virtual void NotifyCwbDone(int32_t display_id, int32_t status, const LayerBuffer& buffer) = 0;
  virtual void TriggerRefresh(int32_t display_id) = 0;
  virtual void TriggerCwbTeardown(int32_t display_id, bool sync_teardown) {}
};

class CwbManagerInterface {
 public:
  virtual DisplayError CaptureCwb(uint32_t display_id, const CwbClient client,
                          const LayerBuffer &output_buffer, const CwbConfig &config,
                          CwbCallback *cwb_callback) = 0;
  virtual void TeardownCwb(uint32_t display_id) {}
  virtual bool HasPendingCwbRequest(uint32_t display_id) { return false; }
  virtual ~CwbManagerInterface() {}
};

}  // namespace sdm

#endif  //  __CWB_MANAGER_INTERFACE_H__
