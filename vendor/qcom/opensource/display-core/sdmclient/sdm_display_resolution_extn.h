/*
* Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#ifndef __HWC_DISPLAY_RESOLUTION_EXTN_H__
#define __HWC_DISPLAY_RESOLUTION_EXTN_H__

#include <stdio.h>
#include <utils/constants.h>
#include <utils/debug.h>
#include <utils/utils.h>

#include <sstream>
#include <string>

namespace sdm {
class SDMDisplayResolutionExtn {
 public:
  SDMDisplayResolutionExtn() {};
  ~SDMDisplayResolutionExtn() {};
  DisplayError GetExtendedDisplayResolutions(uint32_t panel_width, uint32_t panel_height,
                               std::vector<std::pair<uint32_t, uint32_t>> *extended_disp_res);
};

}  // namespace sdm

#endif  // __HWC_DISPLAY_RESOLUTION_EXTN_H__