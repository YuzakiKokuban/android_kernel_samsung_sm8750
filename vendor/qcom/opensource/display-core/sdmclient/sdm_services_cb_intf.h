/*
 * Copyright (c) 2017-2021, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *    * Neither the name of The Linux Foundation. nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
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
 * Changes from Qualcomm Innovation Center, Inc. are provided under the
 * following license:
 *
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#ifndef __SDM_SERVICES_CB_INTF_H__
#define __SDM_SERVICES_CB_INTF_H__

#include <utils/locker.h>

#include "sdm_display.h"

namespace sdm {

class SDMServicesCbIntf {
public:
  virtual ~SDMServicesCbIntf() {}

  virtual void Refresh(uint64_t disp_idx) = 0;
  virtual SDMDisplay *GetDisplayFromClientId(Display id) = 0;
  virtual bool GetComposerStatus() = 0;
  virtual CoreInterface *GetCoreIntf() = 0;
  virtual int &GetIdlePcRefCnt() = 0;
  virtual DisplayError WaitForCommitDone(Display display, int client_id) = 0;
  virtual DisplayError SetDemuraConfig(Display display, int32_t demura_idx) = 0;
  virtual DisplayError SetDemuraState(Display display, int32_t state) = 0;
  virtual DisplayError SetVsyncEnabled(Display display, bool enabled) = 0;
  virtual DisplayError SetDimmingEnable(Display display,
                                        int32_t int_enabled) = 0;
  virtual DisplayError SetDimmingMinBl(Display display, int32_t min_bl) = 0;
  virtual DisplayError SetDisplayBrightness(Display display,
                                            float brightness) = 0;
  virtual DisplayError GetDisplayPortId(uint32_t disp_id, int *port_id) = 0;
  virtual std::mutex *GetLumMutex() = 0;
  virtual DisplayError SetPanelFeatureConfig(Display display, int32_t type, void *data) = 0;
};

} // namespace sdm

#endif // __SDM_SERVICES_CB_INTF_H__
