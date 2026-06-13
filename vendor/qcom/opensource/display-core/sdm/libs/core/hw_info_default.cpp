/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "hw_info_default.h"

#include <vector>
#include <map>
#include <string>

#define __CLASS__ "HWInfoDefault"

namespace sdm {

DisplayError HWInfoDefault::Init() {
  return kErrorNotSupported;
}

DisplayError HWInfoDefault::GetHWResourceInfo(HWResourceInfo *hw_resource) {
  return kErrorNotSupported;
}

DisplayError HWInfoDefault::GetFirstDisplayInterfaceType(HWDisplayInterfaceInfo *hw_disp_info) {
  hw_disp_info->type = kBuiltIn;
  hw_disp_info->is_connected = true;

  return kErrorNone;
}

DisplayError HWInfoDefault::GetDisplaysStatus(HWDisplaysInfo *hw_displays_info) {
  HWDisplayInfo hw_info = {};
  hw_info.display_type = kBuiltIn;
  hw_info.is_connected = 1;
  hw_info.is_primary = 1;
  hw_info.is_wb_ubwc_supported = 0;
  hw_info.display_id = 1;
  (*hw_displays_info)[hw_info.display_id] = hw_info;
  DLOGI("display: %4d-%d, connected: %s, primary: %s", hw_info.display_id, hw_info.display_type,
        hw_info.is_connected ? "true" : "false", hw_info.is_primary ? "true" : "false");
  return kErrorNone;
}

DisplayError HWInfoDefault::GetMaxDisplaysSupported(const SDMDisplayType type, int32_t *max_displays) {
  switch (type) {
    case kPluggable:
    case kVirtual:
      *max_displays = 0;
      break;
    case kBuiltIn:
    case kDisplayTypeMax:
      *max_displays = 1;
      break;
    default:
      DLOGE("Unknown display type %d.", type);
      return kErrorParameters;
  }

  DLOGI("Max %d concurrent displays.", 1);
  DLOGI("Max %d concurrent displays of type %d (BuiltIn).", 1, kBuiltIn);
  DLOGI("Max %d concurrent displays of type %d (Pluggable).", 0, kPluggable);
  DLOGI("Max %d concurrent displays of type %d (Virtual).", 0, kVirtual);

  return kErrorNone;
}

DisplayError HWInfoDefault::GetRequiredDemuraFetchResourceCount(
    std::map<uint32_t, uint8_t> *required_demura_fetch_cnt) {
  return kErrorNotSupported;
}

DisplayError HWInfoDefault::GetDemuraPanelIds(std::vector<uint64_t> *panel_ids) {
  return kErrorNotSupported;
}

DisplayError HWInfoDefault::GetPanelBootParamString(std::string *panel_boot_param_string) {
  return kErrorNotSupported;
}

uint32_t HWInfoDefault::GetMaxMixerCount() {
  return 0;
}

}  // namespace sdm
