/* Copyright (c) 2020, The Linux Foundataion. All rights reserved.
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
 * ​Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear*
 */

#ifndef __PANEL_FEATURE_PROPERTY_INTF_H__
#define __PANEL_FEATURE_PROPERTY_INTF_H__

#include <utils/constants.h>
#include <inttypes.h>

namespace sdm {

/* enumeration of all panel feature related properties */
enum PanelFeaturePropertyID {
  kPanelFeatureDsppIndex,
  kPanelFeatureDsppSPRInfo,
  kPanelFeatureDsppDemuraInfo,
  kPanelFeatureDsppRCInfo,
  kPanelFeatureSPRInitCfg,
  kPanelFeatureSPRPackType,
  kPanelFeatureSPRPackTypeMode,
  kPanelFeatureDemuraInitCfg,
  kPanelFeatureRCInitCfg,
  kPanelFeatureDemuraPanelId,
  kPanelFeatureSPRUDCCfg,
  kPanelFeatureDemuraCfg0Param2,
  kPanelFeatureAiqeSsrcConfig,
  kPanelFeatureAiqeSsrcData,
  kPanelFeatureAIScalerCfg,
  kPanelFeatureAiqeMdnie,
  kPanelFeatureAiqeMdnieArt,
  kPanelFeatureAiqeMdnieIPC,
  kPanelFeatureAiqeCopr,
  kPanelFeatureABCCfg,
  kPanelFeatureDemuraBacklight,
  kPanelFeaturePropertyIDMax
};

struct PanelFeaturePropertyInfo {
  PanelFeaturePropertyID prop_id = kPanelFeaturePropertyIDMax;
  uint64_t prop_ptr = 0;  // Pointer to property data structure
  uint32_t prop_size = 0;  // Size of the property in bytes
  uint32_t version = 0;
};

class PanelFeaturePropertyIntf {
 public:
  virtual ~PanelFeaturePropertyIntf() {}
  virtual int GetPanelFeature(PanelFeaturePropertyInfo *feature_info) = 0;
  virtual int SetPanelFeature(const PanelFeaturePropertyInfo &feature_info) = 0;
};

}  // namespace sdm

#endif  // __PANEL_FEATURE_PROPERTY_INTF_H__

