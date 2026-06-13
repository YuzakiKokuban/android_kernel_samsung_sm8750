/* Copyright (c) 2015, 2020-2021, The Linux Foundation. All rights reserved.
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
 *
 */
/*
 * Changes from Qualcomm Innovation Center, Inc. are provided under the
 * following license:
 *
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#ifndef __CPUHINT_H__
#define __CPUHINT_H__

#include <core/sdm_types.h>
#include <utils/sys.h>
#include "sdm_common.h"
#include "sdm_compositor_callbacks.h"

namespace sdm {

enum PerfHintThreadType {
  kInvalid = 0,
  kSurfaceFlinger,
  kRenderEngine,
  kSDM,
};

enum PerfHintStatus {
  kInactive = 0,
  kActive,
  kRenew,
};

struct LongTermHintInfo {
  int handle_id = 0;
  int tid = 0;
  nsecs_t start_time = 0;
  PerfHintStatus status = kInactive;
};

class SDMDebugHandler;

class CPUHint {
public:
 DisplayError Init(SDMDebugHandler *debug_handler, SDMCompositorCallbacks *cb);
 int ReqHintsOffload(int hint, int tid);
 int ReqHintRelease();
 int ReqHint(PerfHintThreadType type, int tid);
 void ReqEvent(int event);

private:
  const int kLargeComposition = 0x00001097;
  const int kHintPassPid = 0x0000109C; // Inform mpctl about the TID
  const int kPassPidSuccess = -2;      // Check if mpctl received the TID

  bool enabled_ = false;
  DynLib vendor_ext_lib_;
  int (*fn_perf_hint_acq_rel_offload_)(int handle, int hint, const char *pkg,
                                       int duration, int type, int num_args,
                                       int list[]) = NULL;
  int (*fn_perf_hint_)(int hint, const char *pkg, int duration,
                       int type) = NULL;
  int (*fn_perf_lock_rel_offload_)(int handle) = NULL;
  void (*fn_perf_event_)(int event_id, const char *pkg, int num_args,
                         int list[]) = NULL;
  std::mutex tid_lock_;

  LongTermHintInfo large_comp_cycle_{};

  SDMCompositorCallbacks *cb_ = nullptr;
};

} // namespace sdm

#endif // __CPUHINT_H__
