/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#ifndef __SDM_DISPLAY_BUILDER_CB_INTF_H__
#define __SDM_DISPLAY_BUILDER_CB_INTF_H__

#include "sdm_display.h"

namespace sdm {

class SDMDisplayBuilderCbIntf {
public:
  virtual ~SDMDisplayBuilderCbIntf() {}

  virtual DisplayError WaitForResources(bool wait_for_resources,
                                        Display active_builtin_id,
                                        Display display_id) = 0;

  virtual SDMDisplay *GetDisplayFromClientId(Display id) = 0;
  virtual void SetDisplayByClientId(Display id, SDMDisplay *disp) = 0;
  virtual DisplayError SetPowerMode(uint64_t display, int32_t int_mode) = 0;
  virtual DisplayError WaitForCommitDone(Display display, int client_id) = 0;
  virtual void NotifyDisplayAttributes(Display display, Config config) = 0;
  virtual void GetHpdData(int *hpd_bpp, int *hpd_pattern,
                          int *hpd_connected) = 0;
  virtual bool IsClientConnected() = 0;

  std::mutex command_seq_mutex_;
};

} // namespace sdm

#endif // __SDM_DISPLAY_BUILDER_CB_INTF_H__
