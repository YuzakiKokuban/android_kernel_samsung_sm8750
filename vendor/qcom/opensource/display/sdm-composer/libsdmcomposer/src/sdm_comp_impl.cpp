/*
* Copyright (c) 2020-2021, The Linux Foundation. All rights reserved.
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
Changes from Qualcomm Innovation Center are provided under the following license:
Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted (subject to the limitations in the
disclaimer below) provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.

    * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <string.h>

#include <thread>

#include "sdm_comp_impl.h"
#include "core/sdm_types.h"
#include "debug_handler.h"
#include "core/ipc_interface.h"
#include "vm_interface.h"
#include "private/generic_payload.h"
#include "sdm_comp_debugger.h"

#define __CLASS__ "SDMCompImpl"

namespace sdm {

SDMCompImpl *SDMCompImpl::sdm_comp_impl_ = nullptr;
SDMCompDisplayBuiltIn *SDMCompImpl::display_builtin_[kSDMCompDisplayTypeMax] = { nullptr };
uint32_t SDMCompImpl::ref_count_ = 0;
uint32_t SDMCompImpl::disp_ref_count_[kSDMCompDisplayTypeMax] = { 0 };
recursive_mutex recursive_mutex_;

SDMCompImpl *SDMCompImpl::GetInstance() {
  if (!sdm_comp_impl_) {
    sdm_comp_impl_= new SDMCompImpl();
  }

  return sdm_comp_impl_;
}

int SDMCompImpl::Init() {
  lock_guard<recursive_mutex> obj(recursive_mutex_);
  if (ref_count_) {
    ref_count_++;
    return 0;
  }
  int ret = SDMCompServiceIntf::Create(this, &sdm_comp_service_intf_);
  if (ret != 0) {
    DLOGW("SDMCompServiceIntf create failed!! %d\n", ret);
    return ret;
  }

  ipc_intf_ = std::make_shared<SDMCompIPCImpl>();
  DisplayError error = CoreInterface::CreateCore(&buffer_allocator_, &buffer_sync_handler_, NULL,
                                                 ipc_intf_, &core_intf_);
  if (error != kErrorNone) {
    DLOGE("Failed to create CoreInterface");
    SDMCompServiceIntf::Destroy(sdm_comp_service_intf_);
    return -EINVAL;
  }
  ref_count_++;

  SDMCompDebugHandler *sdm_comp_dbg_handler =
                       static_cast<SDMCompDebugHandler *>(SDMCompDebugHandler::Get());
  sdm_comp_dbg_handler->GetProperty(ENABLE_ROUNDED_CORNER, &rc_enabled_);
  DLOGI("RC status: %d", rc_enabled_);

  return 0;
}

int SDMCompImpl::Deinit() {
  lock_guard<recursive_mutex> obj(recursive_mutex_);
  if (ref_count_) {
    ref_count_--;
    if (!ref_count_) {
      if (sdm_comp_service_intf_) {
        SDMCompServiceIntf::Destroy(sdm_comp_service_intf_);
      }

      DisplayError error = CoreInterface::DestroyCore();
      if (error != kErrorNone) {
        DLOGE("Display core de-initialization failed. Error = %d", error);
        return -EINVAL;
      }
      if (ipc_intf_) {
        ipc_intf_->Deinit();
      }
      delete sdm_comp_impl_;
      sdm_comp_impl_ = nullptr;
    }
  }
  return 0;
}

int SDMCompImpl::CreateDisplay(SDMCompDisplayType display_type, CallbackInterface *callback,
                               Handle *disp_hnd) {
  lock_guard<recursive_mutex> obj(recursive_mutex_);
  if (!disp_hnd || display_type >= kSDMCompDisplayTypeMax) {
    return -EINVAL;
  }

  if (display_builtin_[display_type]) {
    *disp_hnd = display_builtin_[display_type];
    disp_ref_count_[display_type]++;
    return 0;
  }
  int status = 0;

  HWDisplaysInfo hw_displays_info = {};
  DisplayError error = core_intf_->GetDisplaysStatus(&hw_displays_info);
  if (error != kErrorNone) {
    DLOGE("Failed to get connected display list. Error = %d", error);
    return -EINVAL;
  }

  for (auto &iter : hw_displays_info) {
    auto &info = iter.second;

    if (info.display_type != kBuiltIn) {
      continue;
    }

    if ((display_type == kSDMCompDisplayTypePrimary && !info.is_primary) ||
        (display_type != kSDMCompDisplayTypePrimary && info.is_primary)) {
      continue;
    }

    if (!info.is_connected) {
      continue;
    }

    DLOGI("Create builtin display, id = %d, type = %d", info.display_id, display_type);
    SDMCompDisplayBuiltIn *display_builtin = new SDMCompDisplayBuiltIn(core_intf_, callback,
                                                               display_type, info.display_id);
    status = display_builtin->Init();
    if (status) {
      delete display_builtin;
      display_builtin = nullptr;
      return status;
    }
    *disp_hnd = display_builtin;
    display_builtin_[display_type] = display_builtin;
    disp_ref_count_[display_type]++;
    HandlePendingEvents();
    break;
  }

  return status;
}

int SDMCompImpl::DestroyDisplay(Handle disp_hnd) {
  lock_guard<recursive_mutex> obj(recursive_mutex_);
  if (!disp_hnd) {
    DLOGE("Display handle is NULL");
    return -EINVAL;
  }
  SDMCompDisplayBuiltIn *sdm_comp_display = reinterpret_cast<SDMCompDisplayBuiltIn *>(disp_hnd);
  SDMCompDisplayType disp_type = sdm_comp_display->GetDisplayType();
  if (disp_ref_count_[disp_type]) {
    disp_ref_count_[disp_type]--;
    if (!disp_ref_count_[disp_type]) {
      int status = sdm_comp_display->Deinit();
      if (status != 0) {
        return status;
      }
      DLOGI("Destroying builtin display %d", disp_type);
      delete display_builtin_[disp_type];
      display_builtin_[disp_type] = nullptr;
    }
  }
  first_commit_ = true;

  return 0;
}

int SDMCompImpl::GetDisplayAttributes(Handle disp_hnd,
                                          SDMCompDisplayAttributes *display_attributes) {
  lock_guard<recursive_mutex> obj(recursive_mutex_);
  if (!disp_hnd || !display_attributes) {
    DLOGE("Invalid input param disp_hnd %d, display_attributes %d", disp_hnd, display_attributes);
    return -EINVAL;
  }

  SDMCompDisplayBuiltIn *sdm_comp_display = reinterpret_cast<SDMCompDisplayBuiltIn *>(disp_hnd);
  return sdm_comp_display->GetDisplayAttributes(display_attributes);
}

int SDMCompImpl::ShowBuffer(Handle disp_hnd, BufferHandle *buf_handle, int32_t *retire_fence) {
  lock_guard<recursive_mutex> obj(recursive_mutex_);
  if (!disp_hnd || !buf_handle || !retire_fence) {
    DLOGE("Invalid input param disp_hnd %d, buf_handle %d, retire_fence %d", disp_hnd, buf_handle,
          retire_fence);
    return -EINVAL;
  }

  SDMCompDisplayBuiltIn *sdm_comp_display = reinterpret_cast<SDMCompDisplayBuiltIn *>(disp_hnd);

  // RC resources available after first cycle so enable RC need two commits.
  if(rc_enabled_ && first_commit_) {
    sdm_comp_display->ShowBuffer(buf_handle, retire_fence);
    int ret = buffer_sync_handler_.SyncWait(*retire_fence);
    if (ret) {
      DLOGW("syncwait failed err:%d", ret);
    }

    close(*retire_fence);
    close(buf_handle->consumer_fence_fd);
    first_commit_ = false;
  }

  return sdm_comp_display->ShowBuffer(buf_handle, retire_fence);
}

int SDMCompImpl::SetColorModeWithRenderIntent(Handle disp_hnd, struct ColorMode mode) {
  lock_guard<recursive_mutex> obj(recursive_mutex_);
  if (!disp_hnd) {
    DLOGE("Invalid input param disp_hnd %d", disp_hnd);
    return -EINVAL;
  }

  SDMCompDisplayBuiltIn *sdm_comp_display = reinterpret_cast<SDMCompDisplayBuiltIn *>(disp_hnd);
  return sdm_comp_display->SetColorModeWithRenderIntent(mode);
}

int SDMCompImpl::GetColorModes(Handle disp_hnd, uint32_t *out_num_modes,
                               struct ColorMode *out_modes) {
  lock_guard<recursive_mutex> obj(recursive_mutex_);
  if (!disp_hnd || !out_num_modes || !out_modes) {
    DLOGE("Invalid input param disp_hnd %d, out_num_modes %d, out_modes %d", disp_hnd, out_num_modes,
          out_modes);
    return -EINVAL;
  }

  SDMCompDisplayBuiltIn *sdm_comp_display = reinterpret_cast<SDMCompDisplayBuiltIn *>(disp_hnd);
  return sdm_comp_display->GetColorModes(out_num_modes, out_modes);
}

int SDMCompImpl::SetPanelBrightness(Handle disp_hnd, float brightness_level) {
  if (!disp_hnd) {
    DLOGE("Invalid input param disp_hnd %d", disp_hnd);
    return -EINVAL;
  }

  SDMCompDisplayBuiltIn *sdm_comp_display = reinterpret_cast<SDMCompDisplayBuiltIn *>(disp_hnd);
  return sdm_comp_display->SetPanelBrightness(brightness_level);
}

int SDMCompImpl::SetMinPanelBrightness(Handle disp_hnd, float min_brightness_level) {
  if (!disp_hnd) {
    DLOGE("Invalid input param disp_hnd %d", disp_hnd);
    return -EINVAL;
  }
  SDMCompDisplayBuiltIn *sdm_comp_display = reinterpret_cast<SDMCompDisplayBuiltIn *>(disp_hnd);
  int err = sdm_comp_display->SetMinPanelBrightness(min_brightness_level);
  if (err == 0) {
    SDMCompDisplayType disp_type = sdm_comp_display->GetDisplayType();
    if (panel_brightness_[disp_type] < min_brightness_level) {
      return SetPanelBrightness(disp_hnd, min_brightness_level);
    }
  }
  return err;
}

int SDMCompImpl::OnEvent(SDMCompServiceEvents event, ...) {
  int err = 0;
  va_list arguments;
  va_start(arguments, event);
  switch (event) {
  case kEventSetPanelBrightness: {
    lock_guard<recursive_mutex> obj(recursive_mutex_);
    SDMCompDisplayType disp_type = (SDMCompDisplayType)(va_arg(arguments, int));
    float panel_brightness = FLOAT(va_arg(arguments, double));
    if (display_builtin_[disp_type]) {
      int err = display_builtin_[disp_type]->SetPanelBrightness(panel_brightness);
      DLOGI("Setting panel brightness value %f on display type %d is %s", panel_brightness,
            disp_type, err ? "failed" : "successful");
    } else {
      pending_events_.emplace(std::make_pair(kEventSetPanelBrightness, disp_type));
      panel_brightness_[disp_type] = panel_brightness;
      DLOGI("Cache panel brightness value %f on display type %d", panel_brightness, disp_type);
    }
  } break;

  case kEventSetDisplayConfig: {
    lock_guard<recursive_mutex> obj(recursive_mutex_);
    SDMCompDisplayType disp_type = (SDMCompDisplayType)(va_arg(arguments, int));
    SDMCompServiceDispConfigs *disp_configs =
        reinterpret_cast<SDMCompServiceDispConfigs*>(va_arg(arguments, Handle));
    if (display_builtin_[disp_type]) {
      DLOGW("Setting display config is not supported in the middle of TUI session");
      err = -ENOTSUP;
    } else {
      pending_events_.emplace(std::make_pair(kEventSetDisplayConfig, disp_type));
      disp_configs_[disp_type] = *disp_configs;
      DLOGI("Cache display h_total %d, vtotal %d, fps %d, %s panel for display type %d",
            disp_configs->h_total, disp_configs->v_total, disp_configs->fps,
            disp_configs->smart_panel ? "cmdmode" : "videomode", disp_type);
    }
  } break;

  case kEventImportDemuraBuffers: {
    lock_guard<recursive_mutex> obj(recursive_mutex_);
    SDMCompServiceDemuraBufInfo *demura_buf_info =
        reinterpret_cast<SDMCompServiceDemuraBufInfo*>(va_arg(arguments, Handle));
    if (demura_buf_info && ipc_intf_) {
      GenericPayload pl;
      SDMCompServiceDemuraBufInfo* buffer = nullptr;
      if ((err = pl.CreatePayload<SDMCompServiceDemuraBufInfo>(buffer))) {
        DLOGE("Failed to create payload for BufferInfo, error = %d", err);
        break;
      }
      buffer->hfc_buf_fd = demura_buf_info->hfc_buf_fd;
      buffer->hfc_buf_size = demura_buf_info->hfc_buf_size;
      buffer->panel_id = demura_buf_info->panel_id;
      if ((err = ipc_intf_->SetParameter(kIpcParamSetDemuraBuffer, pl))) {
        DLOGE("Failed to Cache the demura Buffers err %d", err);
        break;
      }
    }
  } break;

  default:
    err = -EINVAL;
    break;
  }
  va_end(arguments);
  return err;
}

void SDMCompImpl::HandlePendingEvents() {
  for (auto pending_event : pending_events_) {
    SDMCompDisplayType display_type = pending_event.second;
    switch (pending_event.first) {
    case kEventSetDisplayConfig: {
      uint32_t num_configs = 0;
      bool foundConfig = false;
      int err = display_builtin_[display_type]->GetNumVariableInfoConfigs(&num_configs);
      for (uint32_t config_idx = 0; config_idx < num_configs; config_idx++) {
        DisplayConfigVariableInfo variable_info = {};
        int err = display_builtin_[display_type]->GetDisplayConfig(config_idx, &variable_info);
        if (err == 0) {
          if (variable_info.h_total != disp_configs_[display_type].h_total ||
              variable_info.v_total != disp_configs_[display_type].v_total ||
              variable_info.fps != disp_configs_[display_type].fps ||
              variable_info.smart_panel != disp_configs_[display_type].smart_panel) {
            continue;
          }

          err = display_builtin_[display_type]->SetDisplayConfig(config_idx);
          if (err != 0) {
            continue;
          }

          foundConfig = true;
          DLOGI("Setting display config idx %d, WxH %dx%d, fps %d, %s panel for display type %d",
                config_idx, variable_info.x_pixels, variable_info.y_pixels, variable_info.fps,
                disp_configs_[display_type].smart_panel ? "cmdmode" : "videomode", display_type);
          display_builtin_[display_type]->SetMixerConfig(disp_configs_[display_type].mixer_width,
                                                         disp_configs_[display_type].mixer_height);
          break;
        }
      }

      // TODO(user): Need to fix why config not coming properly
      if (!foundConfig) {
        DLOGW("Failed to find the current config, set the default config as 0");
        err = display_builtin_[display_type]->SetDisplayConfig(0);
        if (err != 0) {
          DLOGW("Failed to set config to secondary vm");
        }
      }
    } break;

    case kEventSetPanelBrightness: {
      int err = display_builtin_[display_type]->SetPanelBrightness(panel_brightness_[display_type]);
      DLOGI("SetPanelBrightness value %f on display type %d is %s", panel_brightness_[display_type],
             display_type, err ? "failed" : "successful");
    } break;

    default:
      break;
    }
  }
  pending_events_.clear();
}

int SDMCompIPCImpl::SetParameter(IPCParams param, const GenericPayload &in) {
  int ret = 0;
  switch(param) {
  case kIpcParamSetDemuraBuffer: {
    SDMCompServiceDemuraBufInfo *buf_info = nullptr;
    uint32_t sz = 0;
    if ((ret = in.GetPayload(buf_info, &sz))) {
      DLOGE("Failed to get input payload error = %d", ret);
      return ret;
    }
    if (buf_info->hfc_buf_fd > 0) {
      hfc_buf_info_.hfc_buf_fd = buf_info->hfc_buf_fd;
      hfc_buf_info_.hfc_buf_size = buf_info->hfc_buf_size;
      hfc_buf_info_.panel_id = buf_info->panel_id;
    } else {
      DLOGW("Failed to import HFC buffer");
    }
  } break;
  default:
    break;
  }
  return 0;
}

int SDMCompIPCImpl::GetParameter(IPCParams param, GenericPayload *out) {
  (void)param;
  (void)out;
  DLOGE("GetParameter on param %d is not supported", param);
  return -ENOTSUP;
}

int SDMCompIPCImpl::ProcessOps(IPCOps op, const GenericPayload &in, GenericPayload *out) {
  lock_guard<recursive_mutex> obj(recursive_mutex_);
  if (!out) {
    return -EINVAL;
  }
  int ret = 0;
  switch(op) {
  case kIpcOpsImportBuffers: {
    IPCImportBufInParams *buf_in_params = nullptr;
    IPCImportBufOutParams *buf_out_params = nullptr;
    uint32_t sz = 0;
    if ((ret = in.GetPayload(buf_in_params, &sz))) {
      DLOGE("Failed to get input payload error = %d", ret);
      return ret;
    }
    if ((ret = out->GetPayload(buf_out_params, &sz))) {
      DLOGE("Failed to get output payload error = %d", ret);
      return ret;
    }

    if (buf_in_params->req_buf_type == kIpcBufferTypeDemuraHFC) {
      IPCBufferInfo buf;
      buf.fd = hfc_buf_info_.hfc_buf_fd;
      buf.size = hfc_buf_info_.hfc_buf_size;
      buf.panel_id = hfc_buf_info_.panel_id;
      buf.mem_handle = -1;
      buf_out_params->buffers.push_back(buf);
      DLOGI("ProcessOps: hfc fd:%d and size :%u", hfc_buf_info_.hfc_buf_fd,
        hfc_buf_info_.hfc_buf_size);
    } else {
      DLOGE("Invalid buffer type : %d", buf_in_params->req_buf_type);
    }
  } break;

  default:
    break;
  }
  return 0;
}

int SDMCompIPCImpl::Deinit() {
  hfc_buf_info_ = {};
  return 0;
}
}  // namespace sdm
