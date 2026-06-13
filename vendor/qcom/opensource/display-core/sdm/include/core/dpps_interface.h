/*
* Copyright (c) 2018-2019, 2021, The Linux Foundation. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are
* met:
*  * Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
*  * Redistributions in binary form must reproduce the above
*    copyright notice, this list of conditions and the following
*    disclaimer in the documentation and/or other materials provided
*    with the distribution.
*  * Neither the name of The Linux Foundation nor the names of its
*    contributors may be used to endorse or promote products derived
*    from this software without specific prior written permission.
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
* Changes from Qualcomm Innovation Center are provided under the following license:
*
* Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#ifndef __DPPS_INTERFACE_H__
#define __DPPS_INTERFACE_H__

#include <core/sdm_types.h>
#include <core/display_interface.h>
#include <Dataspace.h>
#include <private/panel_feature_property_intf.h>

#include <string>

namespace sdm {

using QtiColorPrimaries = vendor_qti_hardware_display_common_QtiColorPrimaries;
using QtiGammaTransfer = vendor_qti_hardware_display_common_QtiGammaTransfer;

enum DppsOps {
  kDppsSetFeature,
  kDppsGetFeatureInfo,
  kDppsScreenRefresh,
  kDppsPartialUpdate,
  kDppsRequestCommit,
  kDppsGetDisplayInfo,
  kDppsSetPccConfig,
  kDppsOpMax,
};

enum DppsNotifyOps {
  kDppsCommitEvent,
  kDppsColorSpaceEvent,
  kDppsUpdateFpsEvent,
  kDppsHdrPresentEvent,
  kDppsNotifyMax,
};

struct DppsNotifyPayload {
  bool is_primary;
  void *payload;
  uint32_t payload_size;
};

struct DppsBlendSpaceInfo {
  QtiColorPrimaries primaries = QtiColorPrimaries_BT709_5;
  QtiGammaTransfer transfer = QtiTransfer_sRGB;
  bool is_primary;
};

struct DppsDisplayInfo {
  uint32_t width;
  uint32_t height;
  bool is_primary;
  int32_t display_id;
  std::string brightness_base_path;
#if !defined(LINUX_COMPILE) && !defined(WIN32) && !defined(_WIN64) && !defined(__APPLE__)
  SDMDisplayType display_type;
#else
  uint32_t display_type;
#endif
  uint32_t fps;
};

class DppsPropIntf {
 public:
  virtual DisplayError DppsProcessOps(enum DppsOps op, void *payload, size_t size) = 0;

 protected:
  virtual ~DppsPropIntf() { }
};

class DppsInterface {
 public:
  virtual int Init(DppsPropIntf *intf, const std::string &panel_name,
                   DisplayInterface *display_intf, PanelFeaturePropertyIntf *prop_intf) = 0;
  virtual int Deinit() = 0;
  virtual int DppsNotifyOps(enum DppsNotifyOps op, void *payload, size_t size) = 0;

 protected:
  virtual ~DppsInterface() { }
};

class DppsDummyImpl : public DppsInterface {
 public:
  int Init(DppsPropIntf *intf, const std::string &panel_name, DisplayInterface *display_intf,
           PanelFeaturePropertyIntf *prop_intf = nullptr) {
    (void)intf;
    (void)panel_name;
    (void)display_intf;
    (void)prop_intf;
    return 0;
  }
  int Deinit() {
    delete this;
    return 0;
  }
  int DppsNotifyOps(enum DppsNotifyOps op, void *payload, size_t size) {
    (void)op;
    (void)payload;
    (void)size;
    return 0;
  }
};

}  // namespace sdm

#endif  // __DPPS_INTERFACE_H__
