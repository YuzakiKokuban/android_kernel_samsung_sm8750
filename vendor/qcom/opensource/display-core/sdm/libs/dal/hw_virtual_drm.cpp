/*
Copyright (c) 2017-2021, The Linux Foundation. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.
    * Neither the name of The Linux Foundation nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*
* Changes from Qualcomm Innovation Center are provided under the following license:
*
* Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

/*
* Changes from Qualcomm Innovation Center are provided under the following license:
*
* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include <stdio.h>
#include <ctype.h>
#include <drm_logger.h>
#include <utils/debug.h>
#include <utils/utils.h>
#include <algorithm>
#include <vector>
#include <cmath>
#include "hw_device_drm.h"
#include "hw_virtual_drm.h"
#include "hw_info_drm.h"

#define __CLASS__ "HWVirtualDRM"

using std::vector;

using sde_drm::DRMDisplayType;
using sde_drm::DRMConnectorInfo;
using sde_drm::DRMRect;
using sde_drm::DRMOps;
using sde_drm::DRMPowerMode;
using sde_drm::DRMSecureMode;

namespace sdm {

HWVirtualDRM::HWVirtualDRM(int32_t display_id, BufferAllocator *buffer_allocator,
                           HWInfoInterface *hw_info_intf)
  : HWDeviceDRM(buffer_allocator, hw_info_intf) {
  HWDeviceDRM::device_name_ = "Virtual";
  HWDeviceDRM::disp_type_ = DRMDisplayType::VIRTUAL;
  HWDeviceDRM::display_id_ = display_id;
  HWDeviceDRM::core_id_ = hw_info_intf->GetCoreId();
}

void HWVirtualDRM::ConfigureWbConnectorFbId(uint32_t fb_id) {
  drm_atomic_intf_->Perform(DRMOps::CONNECTOR_SET_OUTPUT_FB_ID, token_.conn_id, fb_id);
  return;
}

void HWVirtualDRM::ConfigureWbConnectorDestRect(bool reset) {
  DRMRect dst = {};

  if (!reset) {
    dst.bottom = display_attributes_[current_mode_index_].y_pixels;
    dst.right = display_attributes_[current_mode_index_].x_pixels;
  }

  drm_atomic_intf_->Perform(DRMOps::CONNECTOR_SET_OUTPUT_RECT, token_.conn_id, dst);
  return;
}

void HWVirtualDRM::ConfigureWbConnectorSecureMode(bool secure) {
  DRMSecureMode secure_mode = secure ? DRMSecureMode::SECURE : DRMSecureMode::NON_SECURE;
  drm_atomic_intf_->Perform(DRMOps::CONNECTOR_SET_FB_SECURE_MODE, token_.conn_id, secure_mode);
}

void HWVirtualDRM::InitializeConfigs() {
  display_attributes_.resize(connector_info_.modes.size());
  for (uint32_t i = 0; i < connector_info_.modes.size(); i++) {
    PopulateDisplayAttributes(i);
  }
}

DisplayError HWVirtualDRM::SetWbConfigs(const HWDisplayAttributes &display_attributes) {
  if (display_attributes.x_pixels > connector_info_.max_linewidth) {
    DLOGE("Requested width %d is more than supported %d", display_attributes.x_pixels,
           connector_info_.max_linewidth);
    return kErrorHardware;
  }

  drmModeModeInfo mode = {};
  vector<drmModeModeInfo> modes;

  mode.hdisplay = mode.hsync_start = mode.hsync_end = mode.htotal =
                                       UINT16(display_attributes.x_pixels);
  mode.vdisplay = mode.vsync_start = mode.vsync_end = mode.vtotal =
                                       UINT16(display_attributes.y_pixels);
  mode.vrefresh = UINT32(display_attributes.fps);

  mode.clock = std::round(FLOAT(mode.htotal * mode.vtotal * mode.vrefresh) / 1000.00f);
  snprintf(mode.name, DRM_DISPLAY_MODE_LEN, "%dx%d", mode.hdisplay, mode.vdisplay);
  modes.push_back(mode);
  for (auto &item : connector_info_.modes) {
    modes.push_back(item.mode);
  }

  // Inform the updated mode list to the driver
  struct sde_drm_wb_cfg wb_cfg = {};
  wb_cfg.connector_id = token_.conn_id;
  wb_cfg.flags = SDE_DRM_WB_CFG_FLAGS_CONNECTED;
  wb_cfg.count_modes = UINT32(modes.size());
  wb_cfg.modes = (uint64_t)modes.data();

  int ret = -EINVAL;
#ifdef DRM_IOCTL_SDE_WB_CONFIG
  ret = drmIoctl(dev_fd_, DRM_IOCTL_SDE_WB_CONFIG, &wb_cfg);
#endif
  if (ret) {
    DLOGE("Dump WBConfig: mode_count %d flags %x", wb_cfg.count_modes, wb_cfg.flags);
    DumpConnectorModeInfo();
    return kErrorHardware;
  }

  return kErrorNone;
}

void HWVirtualDRM::ConfigureDNSC(HWLayersInfo *hw_layers_info) {
#ifdef FEATURE_DNSC_BLUR
  sde_drm::DRMFrameTriggerMode trigger_mode = sde_drm::DRMFrameTriggerMode::FRAME_DONE_WAIT_DEFAULT;
  sde_drm::DRMWBUsageType usage_mode = sde_drm::DRMWBUsageType::WB_USAGE_WFD;

  if (hw_layers_info->iwe_enabled) {
    trigger_mode = sde_drm::DRMFrameTriggerMode::FRAME_DONE_WAIT_POSTED_START;
    usage_mode = sde_drm::DRMWBUsageType::WB_USAGE_OFFLINE_WB;
    topology_control_ |= UINT32(sde_drm::DRMTopologyControl::DNSC_BLUR);
  }

  HWDNSCInfo& dnsc = hw_layers_info->dnsc_cfg;
  dnsc_cfg_ = {};

  if (dnsc.enabled) {
    dnsc_cfg_.flags = dnsc.flags;
    dnsc_cfg_.num_blocks = dnsc.num_blocks;

    dnsc_cfg_.src_width = dnsc.src_width;
    dnsc_cfg_.src_height = dnsc.src_height;
    dnsc_cfg_.dst_width = dnsc.dst_width;
    dnsc_cfg_.dst_height = dnsc.dst_height;

    dnsc_cfg_.flags_h = dnsc.flags_h;
    dnsc_cfg_.flags_v = dnsc.flags_v;

    dnsc_cfg_.phase_init_h = dnsc.pcmn_data.phase_init_h;
    dnsc_cfg_.phase_step_h = dnsc.pcmn_data.phase_step_h;
    dnsc_cfg_.phase_init_v = dnsc.pcmn_data.phase_init_v;
    dnsc_cfg_.phase_step_v = dnsc.pcmn_data.phase_step_v;

    dnsc_cfg_.norm_h = dnsc.gaussian_data.norm_h;
    dnsc_cfg_.ratio_h = dnsc.gaussian_data.ratio_h;
    dnsc_cfg_.norm_v = dnsc.gaussian_data.norm_v;
    dnsc_cfg_.ratio_v = dnsc.gaussian_data.ratio_v;

    for (int i = 0; i < DNSC_BLUR_COEF_NUM && i < dnsc.gaussian_data.coef_hori.size(); i++) {
      dnsc_cfg_.coef_hori[i] = dnsc.gaussian_data.coef_hori[i];
    }

    for (int i = 0; i < DNSC_BLUR_COEF_NUM && i < dnsc.gaussian_data.coef_vert.size(); i++) {
      dnsc_cfg_.coef_vert[i] = dnsc.gaussian_data.coef_vert[i];
    }
  }

  uint32_t conn_id = token_.conn_id;
  drm_atomic_intf_->Perform(DRMOps::CONNECTOR_CACHE_STATE, conn_id, dnsc.cache_state);
  drm_atomic_intf_->Perform(DRMOps::CONNECTOR_EARLY_FENCE_LINE, conn_id, dnsc.early_fence_line);
  drm_atomic_intf_->Perform(DRMOps::CONNECTOR_DNSC_BLR, conn_id, &dnsc_cfg_);
  drm_atomic_intf_->Perform(DRMOps::CONNECTOR_WB_USAGE_TYPE, conn_id, usage_mode);
  drm_atomic_intf_->Perform(DRMOps::CONNECTOR_SET_FRAME_TRIGGER, conn_id, trigger_mode);
#endif
}

DisplayError HWVirtualDRM::Commit(HWLayersInfo *hw_layers_info) {
  std::shared_ptr<LayerBuffer> output_buffer = hw_layers_info->output_buffer;
  if (output_buffer == nullptr) {
    return kErrorUndefined;
  }
  DisplayError err = kErrorNone;
  bool fb_modified = false;

  registry_.Register(hw_layers_info);
  registry_.MapOutputBufferToFbId(output_buffer, &fb_modified);
  uint32_t fb_id = registry_.GetOutputFbId(output_buffer->handle_id);

  if (fb_modified) {
    hw_layers_info->common_info->updates_mask.set(kUpdateFBObject);
  }

  ConfigureWbConnectorFbId(fb_id);
  ConfigureWbConnectorSecureMode(output_buffer->flags.secure);
  ConfigureDNSC(hw_layers_info);
  ConfigureWbConnectorDestRect(hw_layers_info->iwe_enabled);
  // Reset the ROI which may have been previously set by CWB. Need revisit when ROI enabled on
  // virtual.
  ResetROI();
  err = HWDeviceDRM::AtomicCommit(hw_layers_info);
  if (err != kErrorNone) {
    DLOGE("Atomic commit failed for crtc_id %d conn_id %d encoder_id %d", token_.crtc_id,
          token_.conn_id, token_.encoder_id);
  }

  // Retire fence marks WB done event.
  output_buffer->release_fence = hw_layers_info->retire_fence;
  hw_layers_info->output_fb_id = fb_id;

  return(err);
}

DisplayError HWVirtualDRM::Flush(HWLayersInfo *hw_layers_info) {
  DisplayError err = kErrorNone;
  err = Commit(hw_layers_info);

  if (err != kErrorNone) {
    return err;
  }

  return kErrorNone;
}

DisplayError HWVirtualDRM::Validate(HWLayersInfo *hw_layers_info) {
  std::shared_ptr<LayerBuffer> output_buffer = hw_layers_info->output_buffer;
  if (output_buffer == nullptr) {
    return kErrorUndefined;
  }
  bool fb_modified = false;

  registry_.MapOutputBufferToFbId(output_buffer, &fb_modified);
  uint32_t fb_id = registry_.GetOutputFbId(output_buffer->handle_id);

  if (fb_modified) {
    hw_layers_info->common_info->updates_mask.set(kUpdateFBObject);
  }

  ConfigureWbConnectorFbId(fb_id);
  ConfigureWbConnectorDestRect();
  ConfigureWbConnectorSecureMode(output_buffer->flags.secure);

  return HWDeviceDRM::Validate(hw_layers_info);
}

DisplayError HWVirtualDRM::SetDisplayAttributes(const HWDisplayAttributes &display_attributes) {
  if (display_attributes.x_pixels == 0 || display_attributes.y_pixels == 0) {
    return kErrorParameters;
  }

  int mode_index = -1;
  int ret = 0;
  GetModeIndex(display_attributes, &mode_index);

  if (mode_index < 0) {
    DisplayError error = SetWbConfigs(display_attributes);
    if (error != kErrorNone) {
      return error;
    }
  }

  // Reload connector info for updated info
  ret = drm_mgr_intf_->GetConnectorInfo(token_.conn_id, &connector_info_);
  if (ret) {
    DLOGE("Failed getting info for connector id %u. Error: %d.", token_.conn_id, ret);
    return kErrorHardware;
  }
  GetModeIndex(display_attributes, &mode_index);

  if (mode_index < 0) {
    DLOGE("Mode not found for resolution %dx%d fps %d", display_attributes.x_pixels,
          display_attributes.y_pixels, UINT32(display_attributes.fps));
    DumpConnectorModeInfo();
    return kErrorNotSupported;
  }

  current_mode_index_ = UINT32(mode_index);
  InitializeConfigs();
  PopulateHWPanelInfo();
  UpdateMixerAttributes();

  DLOGI("New WB Resolution: %dx%d cur_mode_index %d", display_attributes.x_pixels,
        display_attributes.y_pixels, current_mode_index_);

  return kErrorNone;
}

DisplayError HWVirtualDRM::GetPPFeaturesVersion(PPFeatureVersion *vers) {
  return kErrorNone;
}

void HWVirtualDRM::GetModeIndex(const HWDisplayAttributes &display_attributes, int *mode_index) {
  *mode_index = -1;
  for (uint32_t i = 0; i < connector_info_.modes.size(); i++) {
    if (display_attributes.x_pixels == connector_info_.modes[i].mode.hdisplay &&
        display_attributes.y_pixels == connector_info_.modes[i].mode.vdisplay &&
        display_attributes.fps == connector_info_.modes[i].mode.vrefresh) {
      *mode_index = INT32(i);
      break;
    }
  }
}

DisplayError HWVirtualDRM::PowerOn(const HWQosData &qos_data, SyncPoints *sync_points) {
  DTRACE_SCOPED();
  if (!drm_atomic_intf_) {
    DLOGE("DRM Atomic Interface is null!");
    return kErrorUndefined;
  }

  // Since fb id is not available until first draw cycle and driver expects fb id to be set on any
  // commit(null or atomic commit). Need to defer power on for the first cycle.
  if (first_cycle_) {
    return kErrorNone;
  }

  DisplayError err = HWDeviceDRM::PowerOn(qos_data, sync_points);
  if (err != kErrorNone) {
    return err;
  }

  return kErrorNone;
}

DisplayError HWVirtualDRM::GetDisplayIdentificationData(uint8_t *out_port, uint32_t *out_data_size,
                                                        uint8_t *out_data) {
  *out_data_size = 0;
  *out_port = token_.hw_port;

  return kErrorNone;
}

DisplayError HWVirtualDRM::Deinit() {
#ifdef FEATURE_DNSC_BLUR
  sde_drm::DRMFrameTriggerMode trigger_mode = sde_drm::DRMFrameTriggerMode::FRAME_DONE_WAIT_DEFAULT;
  sde_drm::DRMWBUsageType usage_mode = sde_drm::DRMWBUsageType::WB_USAGE_WFD;
  uint32_t conn_id = token_.conn_id;

  dnsc_cfg_ = {};

  drm_atomic_intf_->Perform(DRMOps::CONNECTOR_CACHE_STATE, conn_id, 0);
  drm_atomic_intf_->Perform(DRMOps::CONNECTOR_EARLY_FENCE_LINE, conn_id, 0);
  drm_atomic_intf_->Perform(DRMOps::CONNECTOR_DNSC_BLR, conn_id, &dnsc_cfg_);
  drm_atomic_intf_->Perform(DRMOps::CONNECTOR_WB_USAGE_TYPE, conn_id, usage_mode);
  drm_atomic_intf_->Perform(DRMOps::CONNECTOR_SET_FRAME_TRIGGER, conn_id, trigger_mode);
  drm_atomic_intf_->Perform(DRMOps::CONNECTOR_SET_TOPOLOGY_CONTROL, conn_id, 0);
#endif

  return HWDeviceDRM::Deinit();
}

}  // namespace sdm