/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __HW_INFO_DEFAULT_H__
#define __HW_INFO_DEFAULT_H__

#include <core/core_interface.h>
#include <core/sdm_types.h>
#include <drm_interface.h>
#include <private/hw_info_types.h>
#include <private/hw_info_interface.h>
#include <vector>
#include <map>
#include <string>

namespace sdm {

class HWInfoDefault : public HWInfoInterface {
 public:
  virtual DisplayError Init();
  virtual DisplayError GetHWResourceInfo(HWResourceInfo *hw_resource);
  virtual DisplayError GetFirstDisplayInterfaceType(HWDisplayInterfaceInfo *hw_disp_info);
  virtual DisplayError GetDisplaysStatus(HWDisplaysInfo *hw_displays_info);
  virtual DisplayError GetMaxDisplaysSupported(SDMDisplayType type, int32_t *max_displays);
  virtual DisplayError GetRequiredDemuraFetchResourceCount(
      std::map<uint32_t, uint8_t> *required_demura_fetch_cnt);
  virtual DisplayError GetDemuraPanelIds(std::vector<uint64_t> *panel_ids);
  virtual DisplayError GetPanelBootParamString(std::string *panel_boot_param_string);
  virtual uint32_t GetMaxMixerCount();
  virtual uint32_t GetCoreId() { return 0; }
};

}  // namespace sdm

#endif  // __HW_INFO_DEFAULT_H__
