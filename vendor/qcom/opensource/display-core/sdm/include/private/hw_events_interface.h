/*
* Copyright (c) 2016-2018, 2020-2021 The Linux Foundation. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification, are permitted
* provided that the following conditions are met:
*    * Redistributions of source code must retain the above copyright notice, this list of
*      conditions and the following disclaimer.
*    * Redistributions in binary form must reproduce the above copyright notice, this list of
*      conditions and the following disclaimer in the documentation and/or other materials provided
*      with the distribution.
*    * Neither the name of The Linux Foundation nor the names of its contributors may be used to
*      endorse or promote products derived from this software without specific prior written
*      permission.
*
* THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
* NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
* BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
* OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*
* Changes from Qualcomm Innovation Center are provided under the following license:
* Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#ifndef __HW_EVENTS_INTERFACE_H__
#define __HW_EVENTS_INTERFACE_H__

#include <private/hw_info_types.h>
#include <inttypes.h>
#include <utility>
#include <vector>

namespace sdm {

class HWEventHandler;
class HWInterface;

enum HWEvent {
  VSYNC = 0,
  EXIT,
  CEC_READ_MESSAGE,
  SHOW_BLANK_EVENT,
  THERMAL_LEVEL,
  IDLE_POWER_COLLAPSE,
  PINGPONG_TIMEOUT,
  PANEL_DEAD,
  HW_RECOVERY,
  HISTOGRAM,
  BACKLIGHT_EVENT,
  MMRM,
  POWER_EVENT,
  VM_RELEASE_EVENT,
#ifdef SEC_GC_QC_DYN_CLK
  DYN_CLK_EVENT,
#endif
  HW_EVENT_MAX,
};

class HWEventsInterface {
 public:
  virtual DisplayError Init(DisplayId display_id, SDMDisplayType display_type,
                            HWEventHandler *event_handler,
                            const std::vector<HWEvent> &event_list) = 0;
  virtual DisplayError Deinit() = 0;
  virtual DisplayError SetEventState(HWEvent event, bool enable, void *aux = nullptr) = 0;
#ifdef SEC_GC_QC_VSYNC
  virtual DisplayError GetEventState(bool *enabled, bool *registered, int64_t *timestamp, int64_t *failed_timestamp, int *error) = 0;
#endif

  static DisplayError Create(DisplayId display_id, SDMDisplayType display_type,
                             HWEventHandler *event_handler, const std::vector<HWEvent> &event_list,
                             HWEventsInterface **intf);
  static DisplayError Destroy(HWEventsInterface *intf);

 protected:
  virtual ~HWEventsInterface() { }
};

}  // namespace sdm

#endif  // __HW_EVENTS_INTERFACE_H__

