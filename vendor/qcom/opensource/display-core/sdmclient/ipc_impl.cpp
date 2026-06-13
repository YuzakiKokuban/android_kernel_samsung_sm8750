/*
 *Copyright (c) 2020-2021, The Linux Foundation. All rights reserved.
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
 *    * Neither the name of The Linux Foundation nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 *THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 *WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 *ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 *BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * Changes from Qualcomm Innovation Center, Inc. are provided under the
 * following license:
 *
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include <cstring>
#include <inttypes.h>
#include <utils/sys.h>

#include "ipc_impl.h"
#include "sdm_common.h"

#define __CLASS__ "IPCImpl"

namespace sdm {

DynLib IPCImpl::qrtr_client_lib_ = {};
CreateQrtrClientIntf IPCImpl::create_qrtr_client_intf_ = nullptr;
DestroyQrtrClientIntf IPCImpl::destroy_qrtr_client_intf_ = nullptr;
QRTRClientInterface *IPCImpl::qrtr_client_intf_ = nullptr;
std::mutex IPCImpl::vm_lock_ = {};
int IPCImpl::client_id_ = 0;
bool IPCImpl::server_ready_ = false;
std::map<int, IPCVmCallbackIntf *> IPCImpl::callbacks_ = {};
MemBuf *IPCImpl::mem_buf_ = nullptr;
DynLib IPCImpl::mem_buf_client_lib_ = {};
GetMemBufInterface IPCImpl::GetMemBuf = nullptr;
PutMemBufInterface IPCImpl::PutMembuf = nullptr;

int IPCImpl::Init() {
  if (init_done_) {
    DLOGW("IPC intf already initialized");
    return 0;
  }
  // Try to load extension library & get handle to its interface.
  if (qrtr_client_lib_.Open(QRTR_CLIENT_LIB_NAME)) {
    if (!qrtr_client_lib_.Sym(
            CREATE_QRTR_CLIENT_INTERFACE_NAME,
            reinterpret_cast<void **>(&create_qrtr_client_intf_)) ||
        !qrtr_client_lib_.Sym(
            DESTROY_QRTR_CLIENT_INTERFACE_NAME,
            reinterpret_cast<void **>(&destroy_qrtr_client_intf_))) {
      DLOGW("Unable to load symbols, error = %s", qrtr_client_lib_.Error());
    }
    if (!create_qrtr_client_intf_ || !destroy_qrtr_client_intf_) {
      return -1;
    }

    if (create_qrtr_client_intf_) {
      QRTRConfig qrtr_config = {};
      qrtr_config.server_id = SDM_COMP_SERVICE_ID;
      qrtr_config.server_version = SDM_COMP_SERVICE_VERSION;
      qrtr_config.server_instance = SDM_COMP_SERVICE_INSTANCE;
      int error =
          create_qrtr_client_intf_(qrtr_config, this, &qrtr_client_intf_);
      if (error != 0) {
        DLOGW("Unable to create interface");
      }
    }
  } else {
    DLOGW("Unable to load = %s, error = %s", QRTR_CLIENT_LIB_NAME,
          qrtr_client_lib_.Error());
  }
  if (mem_buf_client_lib_.Open(MEMBUF_CLIENT_LIB_NAME)) {
    if (!mem_buf_client_lib_.Sym(CREATE_MEMBUF_INTERFACE_NAME,
                                 reinterpret_cast<void **>(&GetMemBuf)) ||
        !mem_buf_client_lib_.Sym(DESTROY_MEMBUF_INTERFACE_NAME,
                                 reinterpret_cast<void **>(&PutMembuf))) {
      DLOGW("Unable to load symbols, error = %s", mem_buf_client_lib_.Error());
    }
    if (!GetMemBuf || !PutMembuf) {
      DLOGE("Membuf Symbols not resolved");
      return -1;
    }
  } else {
    DLOGW("Unable to load = %s, error = %s", MEMBUF_CLIENT_LIB_NAME,
          mem_buf_client_lib_.Error());
  }
  int err = GetMemBuf(&mem_buf_);
  if (err != 0) {
    DLOGE("GetMemBuf failed!! %d", err);
    goto cleanup;
  }

  init_done_ = true;
  return 0;
cleanup:
  return Deinit();
}

int IPCImpl::Deinit() {
  if (!init_done_)
    return 0;

  if (mem_buf_) {
    PutMembuf();
  }

  if (destroy_qrtr_client_intf_) {
    int err = destroy_qrtr_client_intf_(qrtr_client_intf_);
    if (err != 0) {
      return err;
    }
    qrtr_client_intf_ = nullptr;
  }
  init_done_ = false;
  return 0;
}

int IPCImpl::SetParameter(IPCParams param, const GenericPayload &in) {
  int ret = 0;
  switch (param) {
  case kIpcParamBacklight: {
    if (qrtr_client_intf_) {
      IPCBacklightParams *backlight_params = nullptr;
      uint32_t sz = 0;
      Command cmd = {};
      if ((ret = in.GetPayload(backlight_params, &sz))) {
        DLOGE("Failed to get input payload error = %d", ret);
        return ret;
      }
      CmdSetBacklight &cmd_bl = cmd.cmd_set_backlight;
      cmd.id = kCmdSetBacklight;
      cmd_bl.brightness = backlight_params->brightness;
      cmd_bl.disp_type = backlight_params->is_primary ? kDisplayTypePrimary
                                                      : kDisplayTypeSecondary1;
      DLOGI("Send brightness level %f, disp_type %d to SVM", cmd_bl.brightness,
            cmd_bl.disp_type);
      ret = qrtr_client_intf_->SendCommand(&cmd, sizeof(Command));
      if (ret != 0) {
        DLOGE("SendCommand %d failed with %d", cmd.id, ret);
        return ret;
      }
    }
  } break;
  case kIpcParamDisplayConfigs: {
    if (qrtr_client_intf_) {
      IPCDisplayConfigParams *disp_configs = nullptr;
      uint32_t sz = 0;
      Command cmd = {};
      if ((ret = in.GetPayload(disp_configs, &sz))) {
        DLOGE("Failed to get input payload error = %d", ret);
        return ret;
      }
      CmdSetDisplayConfigs &cmd_disp_configs = cmd.cmd_set_disp_configs;
      cmd.id = kCmdSetDisplayConfig;
      cmd_disp_configs.h_total = disp_configs->h_total;
      cmd_disp_configs.v_total = disp_configs->v_total;
      cmd_disp_configs.fps = disp_configs->fps;
      cmd_disp_configs.smart_panel = disp_configs->smart_panel;
      cmd_disp_configs.disp_type = disp_configs->is_primary
                                       ? kDisplayTypePrimary
                                       : kDisplayTypeSecondary1;
      cmd_disp_configs.mixer_width = disp_configs->mixer_width;
      cmd_disp_configs.mixer_height = disp_configs->mixer_height;
      DLOGI("Send display configs: h_total %d v_total %d, fps %d, %s panel, "
            "disp_type %d to SVM",
            cmd_disp_configs.h_total, cmd_disp_configs.v_total,
            cmd_disp_configs.fps,
            cmd_disp_configs.smart_panel ? "cmdmode" : "videomode",
            cmd_disp_configs.disp_type);
      ret = qrtr_client_intf_->SendCommand(&cmd, sizeof(Command));
      if (ret != 0) {
        DLOGE("SendCommand %d failed with %d", cmd.id, ret);
        return ret;
      }
    }
  } break;
  case kIpcParamProperties: {
    if (qrtr_client_intf_) {
      IPCSetPropertyParams *prop_configs = nullptr;
      uint32_t sz = 0;
      Command cmd = {};
      if ((ret = in.GetPayload(prop_configs, &sz))) {
        DLOGE("Failed to get input payload error = %d", ret);
        return ret;
      }
      CmdSetProperties &cmd_prop_configs = cmd.cmd_set_properties;
      cmd.id = kCmdSetProperties;
      std::memcpy(cmd_prop_configs.props.property_list,
                  prop_configs->props.property_list,
                  sizeof(prop_configs->props.property_list));
      cmd_prop_configs.props.count = prop_configs->props.count;
      for (int i = 0; i < cmd_prop_configs.props.count; i++) {
        DLOGI("prop idx : %d, name: %s, value :%s\n", i,
              cmd_prop_configs.props.property_list[i].prop_name,
              cmd_prop_configs.props.property_list[i].value);
      }
      ret = qrtr_client_intf_->SendCommand(&cmd, sizeof(Command));
      if (ret != 0) {
        DLOGE("SendCommand %d failed with %d", cmd.id, ret);
        return ret;
      }
    }
  } break;

  case kIpcParamPanelBoot: {
    if (qrtr_client_intf_) {
      IPCPanelBootParams *panel_boot_params = nullptr;
      uint32_t sz = 0;
      Command cmd = {};
      if ((ret = in.GetPayload(panel_boot_params, &sz))) {
        DLOGE("Failed to get input payload for panel_boot_params error = %d",
              ret);
        return ret;
      }
      CmdSetPanelBootParam &cmd_set_panel_boot_param =
          cmd.cmd_set_panel_boot_param;
      cmd.id = kCmdSetPanelBootParams;
      strlcpy(cmd_set_panel_boot_param.panel_boot_string,
              panel_boot_params->panel_boot_string.c_str(),
              sizeof(cmd_set_panel_boot_param.panel_boot_string));
      DLOGI("Sending boot params %s",
            cmd_set_panel_boot_param.panel_boot_string);
      ret = qrtr_client_intf_->SendCommand(&cmd, sizeof(Command));
      if (ret != 0) {
        DLOGE("SendCommand %d failed with %d", cmd.id, ret);
        return ret;
      }
    }
  } break;
  case kIpcParamSetHFCBuffer: {
    if (qrtr_client_intf_) {
      IPCBufferInfo *hfc_buffer = nullptr;
      uint32_t sz = 0;
      Command cmd = {};
      if ((ret = in.GetPayload(hfc_buffer, &sz))) {
        DLOGE("Failed to get input payload for hfc_buffer error = %d", ret);
        return ret;
      }
      CmdExportDemuraBuffer &cmd_export_demura_buffer =
          cmd.cmd_export_demura_buf;
      cmd.id = kCmdExportDemuraBuffers;
      cmd_export_demura_buffer.demura_mem_info.hfc_mem_hdl =
          hfc_buffer->mem_handle;
      cmd_export_demura_buffer.demura_mem_info.hfc_mem_size = hfc_buffer->size;
      cmd_export_demura_buffer.demura_mem_info.panel_id = hfc_buffer->panel_id;

      DLOGI("Sending hfc params %d",
            cmd_export_demura_buffer.demura_mem_info.hfc_mem_hdl);
      ret = qrtr_client_intf_->SendCommand(&cmd, sizeof(Command));
      if (ret != 0) {
        DLOGE("SendCommand %d failed with %d", cmd.id, ret);
        return ret;
      }
    }
  } break;

  default:
    break;
  }
  return 0;
}

int IPCImpl::GetParameter(IPCParams param, GenericPayload *out) {
  (void)param;
  (void)out;
  DLOGE("GetParameter on param %d is not supported", param);
  return -ENOTSUP;
}

int IPCImpl::ProcessExportBuffers(const GenericPayload &in,
                                  GenericPayload *out) {
  if (!out) {
    return -EINVAL;
  }
  int ret = 0;
  if (qrtr_client_intf_) {
    IPCExportBufInParams *buf_in_params = nullptr;
    IPCExportBufOutParams *buf_out_params = nullptr;
    uint32_t sz = 0;

    if ((ret = in.GetPayload(buf_in_params, &sz))) {
      DLOGE("Failed to get input payload for buf_in_params error = %d", ret);
      return ret;
    }
    if ((ret = out->GetPayload(buf_out_params, &sz))) {
      DLOGE("Failed to get input payload for buf_out_params error = %d", ret);
      return ret;
    }
    Command cmd = {};
    cmd.id = kCmdExportDemuraBuffers;
    DemuraMemInfo &demura_mem_info = cmd.cmd_export_demura_buf.demura_mem_info;
    std::map<IPCBufferType, int> exported_fds = {};
    for (auto &buffer : buf_in_params->buffers) {
      auto &buf_type = buffer.first;
      auto export_buf = buffer.second;
      int temp_fd = -1;

      if (export_buf.mem_handle == -1) {
        ret = mem_buf_->Export(export_buf.fd, &temp_fd, &export_buf.mem_handle);
        if (ret != 0) {
          DLOGE("Export failed with %d for buf type %d", ret, buf_type);
          for (auto &temp_fd : exported_fds) {
            if (temp_fd.second > 0) {
              Sys::close_(temp_fd.second);
            }
          }
          return ret;
        }
      }

      if (buf_type == kIpcBufferTypeDemuraHFC) {
        demura_mem_info.hfc_mem_hdl = export_buf.mem_handle;
        demura_mem_info.hfc_mem_size = export_buf.size;
        demura_mem_info.panel_id = export_buf.panel_id;
        demura_mem_info.calib_mem_hdl = -1;
      }
      exported_fds.emplace(buf_type, temp_fd);
    }
    DLOGI("Sending hfc: mem_hdl %ld, size %d panel_id %lu",
          demura_mem_info.hfc_mem_hdl, demura_mem_info.hfc_mem_size,
          demura_mem_info.panel_id);
    ret = qrtr_client_intf_->SendCommand(&cmd, sizeof(Command));
    if (ret != 0) {
      DLOGE("SendCommand %d failed with %d", cmd.id, ret);
      return ret;
    }
    buf_out_params->exported_fds = exported_fds;
  }

  return 0;
}

int IPCImpl::ProcessOps(IPCOps op, const GenericPayload &in,
                        GenericPayload *out) {
  int ret = 0;

  switch (op) {
  case kIpcOpsFilePath: {
    if (!out) {
      return -EINVAL;
    }
    if (!cb_) {
      return -ENODEV;
    }
    ret = cb_->GetDemuraFilePaths(in, out);
  } break;

  case kIpcOpsExportBuffers: {
    if ((ret = ProcessExportBuffers(in, out))) {
      DLOGE("Failed to process Export buffers");
      return ret;
    }
  } break;

  case kIpcOpsRegisterVmCallback: {
    if (!out) {
      return -EINVAL;
    }
    std::lock_guard<std::mutex> obj(vm_lock_);
    IPCVmCallbackIntf **vm_callback = nullptr;
    uint32_t sz = 0;
    if ((ret = in.GetPayload(vm_callback, &sz))) {
      DLOGE("Failed to get input payload for vm_callback error = %d", ret);
      return ret;
    }
    callbacks_.emplace(client_id_, *vm_callback);
    int *cb_hnd = nullptr;
    if ((ret = out->GetPayload(cb_hnd, &sz))) {
      DLOGE("Failed to get output payload for cb_hnd error = %d", ret);
      return ret;
    }
    *cb_hnd = client_id_;
    if (server_ready_) {
      std::thread(IPCImpl::SpawnOnServerReady, client_id_).detach();
    }
    client_id_++;
  } break;

  case kIpcOpsUnRegisterVmCallback: {
    std::lock_guard<std::mutex> obj(vm_lock_);
    int *client_id = nullptr;
    uint32_t sz = 0;
    if ((ret = in.GetPayload(client_id, &sz))) {
      DLOGE("Failed to get input payload error = %d", ret);
      return ret;
    }
    auto it = callbacks_.find(*client_id);
    if (it != callbacks_.end()) {
      callbacks_.erase(it);
    }
  } break;

  default:
    DLOGE("Unsupported IPCOps");
    return -EINVAL;
  }

  return ret;
}

int IPCImpl::OnResponse(void *rsp_buf, size_t rsp_size) {
  if (rsp_size != sizeof(Response)) {
    DLOGE("Mismatch in response size!! %d-%d", rsp_size, sizeof(Response));
    return -EINVAL;
  }
  Response *rsp = reinterpret_cast<Response *>(rsp_buf);
  switch (rsp->id) {
  case kCmdSetBacklight: {
    if (rsp->status != 0) {
      DLOGW("Response for set backlight level failed with status %d",
            rsp->status);
      return rsp->status;
    }
    DLOGI("Response for set backlight level received successfully");
  } break;
  case kCmdSetProperties: {
    if (rsp->status != 0) {
      DLOGW("Response for set properties failed with status %d", rsp->status);
      return rsp->status;
    }
    DLOGI("Response for set properties received successfully");
  } break;
  case kCmdExportDemuraBuffers:
    DLOGI("Response for export demura buffers returned with status %d",
          rsp->status);
    break;

  default:
    break;
  }
  return 0;
}

void IPCImpl::OnServerReady() {
  std::lock_guard<std::mutex> obj(vm_lock_);
  server_ready_ = true;
  for (auto &callback : callbacks_) {
    IPCVmCallbackIntf *vm_callback = callback.second;
    if (vm_callback) {
      vm_callback->OnServerReady();
    }
  }
  DLOGI("LE server is ready");
}

void IPCImpl::OnServerExit() {
  std::lock_guard<std::mutex> obj(vm_lock_);
  server_ready_ = false;
  for (auto &callback : callbacks_) {
    IPCVmCallbackIntf *vm_callback = callback.second;
    if (vm_callback) {
      vm_callback->OnServerExit();
    }
  }
  DLOGI("LE server is exited");
}

void IPCImpl::SpawnOnServerReady(int client_id) {
  if (callbacks_.find(client_id) != callbacks_.end()) {
    if (callbacks_[client_id]) {
      callbacks_[client_id]->OnServerReady();
    }
  }
}

} // namespace sdm
