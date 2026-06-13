/*
* Copyright (c) 2014-2016, 2018 The Linux Foundation. All rights reserved.
*
* Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
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
*
* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#ifndef __HW_INFO_INTERFACE_H__
#define __HW_INFO_INTERFACE_H__

#include <core/core_interface.h>
#include <private/hw_info_types.h>
#include <utils/multi_core_instantiator.h>
#include <inttypes.h>
#include <vector>
#include <utility>
#include <map>
#include <string>

namespace sdm {

class HWInfoInterface {
 public:
  static DisplayError Create(sdm::MultiCoreInstance<uint32_t, HWInfoInterface *> *intfs,
                             std::bitset<8> core_ids);
  static DisplayError Destroy(sdm::MultiCoreInstance<uint32_t, HWInfoInterface *> &intfs);
  virtual DisplayError Init() = 0;
  virtual DisplayError GetHWResourceInfo(HWResourceInfo *hw_resource) = 0;
  virtual DisplayError GetFirstDisplayInterfaceType(HWDisplayInterfaceInfo *hw_disp_info) = 0;
  virtual DisplayError GetDisplaysStatus(HWDisplaysInfo *hw_displays_info) = 0;
  virtual DisplayError GetMaxDisplaysSupported(SDMDisplayType type, int32_t *max_displays) = 0;
  virtual DisplayError GetRequiredDemuraFetchResourceCount(
                       std::map<uint32_t, uint8_t> *required_demura_fetch_cnt) = 0;
  virtual DisplayError GetDemuraPanelIds(std::vector<uint64_t> *panel_ids) = 0;
  virtual DisplayError GetPanelBootParamString(std::string *panel_boot_param_string) = 0;
  virtual uint32_t GetMaxMixerCount() = 0;
  virtual uint32_t GetCoreId() = 0;

 protected:
  static sdm::MultiCoreInstance<uint32_t, HWInfoInterface *> intf_;
  virtual ~HWInfoInterface() { }
  static const int kMaxCore = 12;
  static int32_t ref_count_;
};

}  // namespace sdm

#endif  // __HW_INFO_INTERFACE_H__

