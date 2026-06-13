/*
* Copyright (c) 2020 - 2021, The Linux Foundation. All rights reserved.
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
 * ​Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __DEMURA_INTF_H__
#define __DEMURA_INTF_H__

#include <core/buffer_allocator.h>
#include <core/buffer_sync_handler.h>
#include <core/ipc_interface.h>
#include <core/display_interface.h>

#include <private/generic_intf.h>
#include <private/generic_payload.h>

#include <array>
#include <bitset>
#include <string>
#include <vector>

#define RESOURCE_BITSET 8

namespace sdm {

static const uint32_t kMaxPanelConfigSupported = 4;
static const int kDemuraDefaultIdx = 0;

template <typename T>
struct DemuraFeatureParamConfigIdx {
  T modeinfo;
};

typedef DemuraFeatureParamConfigIdx<uint64_t> uConfigIdx;

enum CorrectionSurface {
  kDemuraCorrSurfaceMain = 0,
  kDemuraCorrSurfaceUdc,
  kDemuraBuffMax,
};

struct DemuraCorrectionSurfaces {
  std::array<BufferInfo, kDemuraBuffMax> surfaces;
  std::array<bool, kDemuraBuffMax> valid;
};

struct DemuraInputConfig {
  bool secure_session = false;
  std::string brightness_path;
  std::bitset<RESOURCE_BITSET> resources;
  int secure_hfc_fd = -1;
  size_t secure_hfc_size = 0;
  uint64_t panel_id = 0;
  uint64_t panel_width = 0;
  uint64_t panel_height = 0;
  std::string panel_name;
  DisplayInterface *display_intf = nullptr;
};

struct DemuraBacklightInfo {
  uint32_t os_brightness_max;
  uint32_t os_brightness;
};

// Demura specific param as strings
const std::string kDemuraFeatureParamActive = "Active";
const std::string kDemuraFeatureParamCorrectionBuffer = "CorrectionBuffer";
const std::string kDemuraFeatureParamPanelId = "PanelId";
const std::string kDemuraFeatureParamPendingReconfig = "PendingReconfig";
const std::string kDemuraFeatureParamSprPattern = "SprPattern";
const std::string kDemuraFeatureParamSprPatternMode = "SprPatternMode";
const std::string kDemuraFeatureParamConfigIdx = "ConfigIdx";
const std::string kDemuraFeatureParamNeedScreenRefresh = "NeedScreenRefresh";
const std::string kDemuraFeatureParamBacklightEvent = "DisplayBacklightEvent";
const std::string kDemuraFeatureParamBrightHeadroomRatio = "BrightHeadroomRatio";
const std::string kDemuraFeatureParamRefreshRate = "RefreshRate";

using DemuraIntf = GenericIntf<const std::string&, const std::string&, GenericPayload>;
}  // namespace sdm

#endif  // __DEMURA_INTF_H__
