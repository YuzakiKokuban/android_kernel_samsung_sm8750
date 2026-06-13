/*
 * Copyright (c) 2020-2021, The Linux Foundation. All rights reserved.
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

#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/mman.h>
#include <systemdq/sd-bus.h>

#include <mutex>
#include <algorithm>
#include <cstring>
#include <string>

#include "sdm_comp_service.h"
#include "sdm_comp_debugger.h"
#include "display_properties.h"

#define __CLASS__ "SDMCompService"

#define DISPLAY_SERVICE_FILE_WITH_PARAM "display@.service"
#define DISPLAY_SERVICE_FILE_NO_PARAM "display.service"
#define SYSTEMD_ESCAPE_BIN "/bin/systemd-escape"

namespace sdm {

std::mutex SDMCompService::qrtr_lock_;
std::vector <SDMCompServiceCbIntf *> SDMCompService::callbacks_ = {};
int SDMCompService::qrtr_fd_ = -1;
DynLib SDMCompService::qrtr_lib_ = {};
MemBuf *SDMCompService::mem_buf_ = nullptr;
DynLib SDMCompService::extension_lib_ = {};
CreateSDMCompExtnIntf SDMCompService::create_sdm_comp_extn_intf_ = nullptr;
DestroySDMCompExtnIntf SDMCompService::destroy_sdm_comp_extn_intf_ = nullptr;
SDMCompServiceExtnIntf *SDMCompService::sdm_comp_service_extn_intf_ = nullptr;
int SDMCompService::exit_thread_fd_ = -1;
std::thread SDMCompService::event_thread_ = {};
QrtrOpen SDMCompService::qrtr_open_ = nullptr;
QrtrClose SDMCompService::qrtr_close_ = nullptr;
QrtrSendTo SDMCompService::qrtr_send_to_ = nullptr;
QrtrPublish SDMCompService::qrtr_publish_ = nullptr;
QrtrBye SDMCompService::qrtr_bye_ = nullptr;
QrtrDecode SDMCompService::qrtr_decode_ = nullptr;
std::mutex SDMCompService::pending_cmd_lock_;
std::multimap<int, struct qrtr_packet> SDMCompService::pending_commands_ = {};

static bool IsModuleLoaded() {
  FILE *fp = popen("cat /proc/modules | grep msm_drm", "r");
  if (fp == NULL) {
    return false;
  }
  char buf[16];
  if (fread (buf, 1, sizeof (buf), fp) > 0) {
    pclose(fp);
    return true;
  }
  pclose(fp);
  return false;
}

static int LoadModule(const std::string &service_file_path, const std::string &argument) {
  if (IsModuleLoaded()) {
    return 0;
  }
  sd_bus_error error = SD_BUS_ERROR_NULL;
  sd_bus_message *msg = NULL;
  sd_bus *bus = NULL;
  char systemd_escape_cmd[256] = {};
  char modified_arg[256] = {};
  std::string mod_service_file_path(service_file_path);
  if (!argument.empty()) {
    FILE *cmd_file;
    snprintf(systemd_escape_cmd, sizeof(systemd_escape_cmd), "%s '%s' 2>&1",
             SYSTEMD_ESCAPE_BIN, argument.c_str());
    DLOGI("systemd_escape_cmd %s", systemd_escape_cmd);
    cmd_file = popen(systemd_escape_cmd, "r");
    if (!cmd_file) {
      DLOGI("cmd file is NULL");
      return -EINVAL;
    }
    if (fgets(modified_arg, sizeof(modified_arg), cmd_file) == NULL) {
      DLOGE("fgets returned null");
      return -EINVAL;
    }
    DLOGI("modified_arg %s", modified_arg);
    pclose(cmd_file);

    std::size_t pos = mod_service_file_path.find_first_of('@');
    if (pos == std::string::npos) {
      DLOGE("Invalid service file path!!");
      return -EINVAL;
    }
    mod_service_file_path.insert(pos + 1, modified_arg);

    pos = mod_service_file_path.find_first_of('\n');
    if (pos != std::string::npos) {
      mod_service_file_path.erase(pos, 1);
    }
  }

  DLOGI("service file path %s", mod_service_file_path.c_str());

  int err = sd_bus_open_system(&bus);
  if (err < 0) {
    DLOGE("Failed to connect to system bus err %d %s", err, strerror(errno));
    return err;
  }
  err = sd_bus_call_method(bus,
                           "org.freedesktop.systemd1",
                           "/org/freedesktop/systemd1",
                           "org.freedesktop.systemd1.Manager",
                           "StartUnit",
                           &error,
                           &msg,
                           "ss",
                           mod_service_file_path.c_str(),
                           "replace");
  if (err < 0) {
    DLOGE("sd_bus_call_method failed with err %d %s", err, strerror(errno));
  }
  sd_bus_error_free(&error);
  sd_bus_message_unref(msg);
  sd_bus_unref(bus);

  DLOGI("Loading kernel module is %s", (err < 0) ? "failed" : "successful");
  return ((err < 0) ? err : 0);
}

int SDMCompService::RegisterCallback(SDMCompServiceCbIntf *callback) {
  std::lock_guard<std::mutex> lock(qrtr_lock_);

  if (qrtr_fd_ > 0) {
    callbacks_.push_back(callback);
    if (callback) {
      std::thread(HandlePendingCommands).detach();
    }
    return 0;
  }

  int err = 0;
  // Try to load qrtr library & get handle to its interface.
  if (qrtr_lib_.Open("libqrtr.so.1.0.0")) {
    if (!qrtr_lib_.Sym("qrtr_open", reinterpret_cast<void **>(&qrtr_open_)) ||
        !qrtr_lib_.Sym("qrtr_close", reinterpret_cast<void **>(&qrtr_close_)) ||
        !qrtr_lib_.Sym("qrtr_sendto", reinterpret_cast<void **>(&qrtr_send_to_)) ||
        !qrtr_lib_.Sym("qrtr_publish", reinterpret_cast<void **>(&qrtr_publish_)) ||
        !qrtr_lib_.Sym("qrtr_bye", reinterpret_cast<void **>(&qrtr_bye_)) ||
        !qrtr_lib_.Sym("qrtr_decode", reinterpret_cast<void **>(&qrtr_decode_))) {
      DLOGE("Unable to load symbols, error = %s", qrtr_lib_.Error());
      return-ENOENT;
    }
  } else {
    DLOGE("Unable to load libqrtr.so, error = %s", qrtr_lib_.Error());
    return-ENOENT;
  }

  qrtr_fd_ = qrtr_open_(0);
  if (qrtr_fd_ < 0) {
    DLOGE("Failed to create qrtr socket");
    err = -EINVAL;
    goto cleanup;
  }

  err = qrtr_publish_(qrtr_fd_, SDM_COMP_SERVICE_ID, SDM_COMP_SERVICE_VERSION,
                      SDM_COMP_SERVICE_INSTANCE);
  if (err < 0) {
    DLOGE("failed to publish rmtfs service %d", err);
    goto cleanup;
  }

  err = MemBuf::GetInstance(&mem_buf_);
  if (err != 0) {
    DLOGE("MemBuf::GetInstance failed!! %d\n", err);
    goto cleanup;
  }

  // Try to load extension library & get handle to its interface.
  if (extension_lib_.Open(EXTN_LIB_NAME)) {
    if (!extension_lib_.Sym(CREATE_SDMCOMP_SERVICE_EXTN,
                            reinterpret_cast<void **>(&create_sdm_comp_extn_intf_)) ||
        !extension_lib_.Sym(DESTROY_SDMCOMP_SERVICE_EXTN,
                            reinterpret_cast<void **>(&destroy_sdm_comp_extn_intf_))) {
      DLOGE("Unable to load symbols, error = %s", extension_lib_.Error());
      err = -ENOENT;
      goto cleanup;
    }

    err = create_sdm_comp_extn_intf_(qrtr_fd_, &sdm_comp_service_extn_intf_);
    if (err != 0) {
      DLOGE("Unable to create sdm comp service extenstion interface");
      goto cleanup;
    }
  } else {
    DLOGW("Unable to load = %s, error = %s", EXTN_LIB_NAME, extension_lib_.Error());
  }
  // Create an eventfd to be used to unblock the poll system call when
  // a thread is exiting.
  exit_thread_fd_ = Sys::eventfd_(0, 0);

  event_thread_ = std::thread(&SDMCompService::QRTREventHandler);
  event_thread_.detach();

  callbacks_.push_back(callback);
  if (callback) {
    HandlePendingCommands();
  }

  return 0;
cleanup:
  UnRegisterCallback(callback);

  return err;
}

int SDMCompService::UnRegisterCallback(SDMCompServiceCbIntf *callback) {
  std::lock_guard<std::mutex> lock(qrtr_lock_);

  auto it = std::find(callbacks_.begin(), callbacks_.end(), callback);
  if (it != callbacks_.end()) {
    callbacks_.erase(it);
  }

  if (!callbacks_.empty()) {
    return 0;
  }

  uint64_t exit_value = 1;
  if (exit_thread_fd_ > 0) {
    ssize_t write_size = Sys::write_(exit_thread_fd_, &exit_value, sizeof(uint64_t));
    if (write_size != sizeof(uint64_t)) {
      DLOGW("Error triggering exit fd (%d). write size = %d, error = %s", exit_thread_fd_,
            write_size, strerror(errno));
    }
    if (event_thread_.joinable()) {
      event_thread_.join();
    }
    Sys::close_(exit_thread_fd_);
  }

  if (destroy_sdm_comp_extn_intf_) {
    destroy_sdm_comp_extn_intf_(sdm_comp_service_extn_intf_);
  }

  if (mem_buf_) {
    MemBuf::PutInstance();
  }

  if (qrtr_fd_ > 0) {
    qrtr_bye_(qrtr_fd_, SDM_COMP_SERVICE_ID, SDM_COMP_SERVICE_VERSION,
              SDM_COMP_SERVICE_INSTANCE);
    qrtr_close_(qrtr_fd_);
  }

  return 0;
}

void SDMCompService::SendResponse(int node, int port, const Response &rsp) {
  int ret = qrtr_send_to_(qrtr_fd_, node, port, &rsp, sizeof(rsp));
  if (ret < 0) {
    DLOGE("Failed to send response for command %d ret %d", rsp.id, ret);
  }
  DLOGI("Sent response for the command id %d size %d", rsp.id, sizeof(rsp));
}

void SDMCompService::HandleImportDemuraBuffers(const struct qrtr_packet &qrtr_pkt, Response *rsp) {
  Command *cmd = reinterpret_cast<Command *>(qrtr_pkt.data);
  rsp->id = cmd->id;
  DemuraMemInfo *demura_mem_info = &cmd->cmd_export_demura_buf.demura_mem_info;
  SDMCompServiceDemuraBufInfo demura_buf_info = {};
  int demura_enabled = 0;

  SDMCompDebugHandler *sdm_comp_dbg_handler =
                       static_cast<SDMCompDebugHandler *>(SDMCompDebugHandler::Get());
  sdm_comp_dbg_handler->GetProperty(ENABLE_DEMURA, &demura_enabled);

  if (!demura_enabled) {
    DLOGI("demura is not enabled on TVM");
    return;
  }

  DLOGI("hfc mem handle :%d", demura_mem_info->hfc_mem_hdl);

  if (demura_mem_info->hfc_mem_hdl != -1) {
    VmParams vm_params;
    std::bitset<kVmPermissionMax> buf_perm = { 0 };
    buf_perm.set(kVmPermissionRead);
    buf_perm.set(kVmPermissionWrite);
    vm_params.emplace(kVmTypeTrusted, buf_perm);
    vm_params.emplace(kVmTypePrimary, buf_perm);

    int error = mem_buf_->Import(demura_mem_info->hfc_mem_hdl, vm_params,
                                 &demura_buf_info.hfc_buf_fd);
    if (error != 0) {
      DLOGW("Import failed with %d", error);
      rsp->status = error;
      return;
    }
    demura_buf_info.hfc_buf_size = demura_mem_info->hfc_mem_size;
    demura_buf_info.panel_id = demura_mem_info->panel_id;

    DLOGI("hfc fd:%d and size :%u", demura_buf_info.hfc_buf_fd, demura_buf_info.hfc_buf_size);
  }

  for (auto callback : callbacks_) {
    if (callback) {
      int err = callback->OnEvent(kEventImportDemuraBuffers, &demura_buf_info);
      DLOGI("ImportDemuraBuffers on panel_id %lu is %s", demura_mem_info->panel_id,
            err ? "failed" : "successful");
      rsp->status = err;
    }
  }
}

void SDMCompService::HandleSetBacklight(const struct qrtr_packet &qrtr_pkt, Response *rsp) {
  Command *cmd = reinterpret_cast<Command *>(qrtr_pkt.data);
  rsp->id = cmd->id;
  CmdSetBacklight *cmd_backlight = reinterpret_cast<CmdSetBacklight*>(&cmd->cmd_set_backlight);
  SDMCompDisplayType sdm_comp_disp_type = GetSDMCompDisplayType(cmd_backlight->disp_type);

  if (sdm_comp_disp_type == kSDMCompDisplayTypeMax) {
    DLOGE("Invalid display_type %d", sdm_comp_disp_type);
    rsp->status = -EINVAL;
    return;
  }
  for (auto callback : callbacks_) {
    if (callback) {
      int err = callback->OnEvent(kEventSetPanelBrightness, sdm_comp_disp_type,
                                  cmd_backlight->brightness);
      rsp->status = err;
    }
  }
}

void SDMCompService::HandleSetDisplayConfigs(const struct qrtr_packet &qrtr_pkt, Response *rsp) {
  Command *cmd = reinterpret_cast<Command *>(qrtr_pkt.data);
  rsp->id = cmd->id;
  CmdSetDisplayConfigs *cmd_disp_configs =
    reinterpret_cast<CmdSetDisplayConfigs*>(&cmd->cmd_set_disp_configs);
  SDMCompDisplayType sdm_comp_disp_type = GetSDMCompDisplayType(cmd_disp_configs->disp_type);

  if (sdm_comp_disp_type == kSDMCompDisplayTypeMax) {
    DLOGE("Invalid display_type %d", sdm_comp_disp_type);
    rsp->status = -EINVAL;
    return;
  }

  for (auto callback : callbacks_) {
    if (callback) {
      SDMCompServiceDispConfigs disp_configs = {};
      disp_configs.h_total = cmd_disp_configs->h_total;
      disp_configs.v_total = cmd_disp_configs->v_total;
      disp_configs.fps = cmd_disp_configs->fps;
      disp_configs.smart_panel = cmd_disp_configs->smart_panel;
      disp_configs.mixer_width = cmd_disp_configs->mixer_width;
      disp_configs.mixer_height = cmd_disp_configs->mixer_height;
      int err = callback->OnEvent(kEventSetDisplayConfig, sdm_comp_disp_type, &disp_configs);
      rsp->status = err;
    }
  }
}

void SDMCompService::HandleSetProperties(const struct qrtr_packet &qrtr_pkt, Response *rsp) {
  Command *cmd = reinterpret_cast<Command *>(qrtr_pkt.data);
  rsp->id = cmd->id;

  CmdSetProperties *cmd_set_props =
    reinterpret_cast<CmdSetProperties *>(&cmd->cmd_set_properties);

  for (int i = 0; i < cmd_set_props->props.count; i++) {
    SDMCompDebugHandler *sdm_comp_dbg_handler =
                         static_cast<SDMCompDebugHandler *>(SDMCompDebugHandler::Get());
    sdm_comp_dbg_handler->SetProperty(cmd_set_props->props.property_list[i].prop_name,
      cmd_set_props->props.property_list[i].value);
    DLOGI("prop idx : %d, name: %s, value :%s", i, cmd_set_props->props.property_list[i].prop_name,
      cmd_set_props->props.property_list[i].value);
  }
}

void SDMCompService::HandleSetPanelBootParams(const struct qrtr_packet &qrtr_pkt, Response *rsp) {
  Command *cmd = reinterpret_cast<Command *>(qrtr_pkt.data);
  rsp->id = cmd->id;

  CmdSetPanelBootParam *cmd_set_panel_boot_param =
    reinterpret_cast<CmdSetPanelBootParam *>(&cmd->cmd_set_panel_boot_param);

  DLOGI("panel_boot_param_string %s", cmd_set_panel_boot_param->panel_boot_string);

  int ret = 0;
  std::string panel_boot_str(cmd_set_panel_boot_param->panel_boot_string);
  if (!panel_boot_str.empty())
    ret = LoadModule(DISPLAY_SERVICE_FILE_WITH_PARAM, panel_boot_str);
  else
    ret = LoadModule(DISPLAY_SERVICE_FILE_NO_PARAM, panel_boot_str);
  if (ret != 0) {
    DLOGE("Failed loading kernel module %d", ret);
    rsp->status = -EINVAL;
    return;
  }
}

void SDMCompService::CommandHandler(const struct qrtr_packet &qrtr_pkt) {
  Response rsp = {};
  rsp.status = -EINVAL;
  if (qrtr_pkt.data_len < sizeof(Command)) {
    DLOGW("Invalid packet!! length %zu command buffer size %d", qrtr_pkt.data_len, sizeof(Command));
    SendResponse(qrtr_pkt.node, qrtr_pkt.port, rsp);
    return;
  }
  Command *cmd = reinterpret_cast<Command *>(qrtr_pkt.data);
  if (!cmd) {
    DLOGE("cmd is null!!\n");
    SendResponse(qrtr_pkt.node, qrtr_pkt.port, rsp);
    return;
  }
  rsp.id = cmd->id;

  if (cmd->id < kCmdMax) {
    if (cmd->version > VM_INTF_VERSION) {
      DLOGE("Invalid vm interface client version %x, supported version %x", cmd->version,
            VM_INTF_VERSION);
      SendResponse(qrtr_pkt.node, qrtr_pkt.port, rsp);
      return;
    }

    // Command kCmdSetPanelBootParams is handled in sdm comp service. So valid client
    // is not needed to handle it.
    if ((cmd->id != kCmdSetPanelBootParams) && (cmd->id != kCmdSetProperties) &&
        !IsRegisteredClientValid()) {
      std::lock_guard<std::mutex> lock(pending_cmd_lock_);
      struct qrtr_packet qrtr_pkt_temp = qrtr_pkt;
      qrtr_pkt_temp.data = (void *)new uint8_t[sizeof(Command)];
      qrtr_pkt_temp.data_len = sizeof(Command);
      memcpy(qrtr_pkt_temp.data, qrtr_pkt.data, qrtr_pkt_temp.data_len);
      pending_commands_.emplace(std::make_pair(cmd->id, qrtr_pkt_temp));
      DLOGI("Registered client invalid handle the command %d later", cmd->id);
      rsp.status = 0;
      SendResponse(qrtr_pkt.node, qrtr_pkt.port, rsp);
      return;
    }
  }

  DLOGI("Received command %d from client fd %d node %d port %d", cmd->id, qrtr_fd_, qrtr_pkt.node,
         qrtr_pkt.port);

  switch (cmd->id) {
    case kCmdExportDemuraBuffers:
      HandleImportDemuraBuffers(qrtr_pkt, &rsp);
      break;
    case kCmdSetBacklight: {
      HandleSetBacklight(qrtr_pkt, &rsp);
    } break;
    case kCmdSetDisplayConfig: {
      HandleSetDisplayConfigs(qrtr_pkt, &rsp);
    } break;
    case kCmdSetProperties: {
      HandleSetProperties(qrtr_pkt, &rsp);
    } break;
    case kCmdSetPanelBootParams: {
      HandleSetPanelBootParams(qrtr_pkt, &rsp);
    } break;
    default:
      if (sdm_comp_service_extn_intf_) {
        sdm_comp_service_extn_intf_->CommandHandler(qrtr_pkt);
      }
      break;
  }

  if (cmd->id < kCmdMax) {
    SendResponse(qrtr_pkt.node, qrtr_pkt.port, rsp);
  }
}

void SDMCompService::QRTREventHandler() {
  struct sockaddr_qrtr soc_qrtr = {};
  struct qrtr_packet qrtr_pkt = {};
  socklen_t soc_len;
  char buf[4096] = {};
  struct pollfd poll_fd[2] = {-1};

  qrtr_lock_.lock();
  // Add qrtr socket fd
  poll_fd[0].fd = qrtr_fd_;
  poll_fd[0].revents = 0;
  poll_fd[0].events = POLLIN | POLLERR;
  qrtr_lock_.unlock();

  // Add event fd to exit the thread
  poll_fd[1].fd = exit_thread_fd_;
  poll_fd[1].revents = 0;
  poll_fd[1].events = POLLIN;
  // Clear any existing data
  Sys::pread_(exit_thread_fd_, buf, 4096, 0);

  DLOGI("Start listening to the client request %d", SDM_COMP_SERVICE_ID);
  while(!callbacks_.empty()) {
    int ret = poll(poll_fd, sizeof(poll_fd) / sizeof(struct pollfd), -1);
    if (ret < 0) {
      continue;
    }

    if (!(poll_fd[0].revents & POLLIN || poll_fd[0].revents & POLLERR)) {
      continue;
    }

    soc_len = sizeof(soc_qrtr);
    ret = recvfrom(qrtr_fd_, buf, sizeof(buf), 0, (sockaddr *)&soc_qrtr,
                   &soc_len);
    if (ret < 0) {
      if (errno == EAGAIN) {
        continue;
      }
      return;
    }

    {
      std::lock_guard<std::mutex> lock(qrtr_lock_);

      ret = qrtr_decode_(&qrtr_pkt, buf, ret, &soc_qrtr);
      if (ret < 0) {
        DLOGE("failed to decode incoming message");
        continue;
      }

      switch (qrtr_pkt.type) {
        case QRTR_TYPE_DEL_CLIENT:
          DLOGI("Client with port %d node %d is disconnected", qrtr_pkt.port, qrtr_pkt.node);
          break;

        case QRTR_TYPE_BYE:
          DLOGI("System server goes down");
          break;

        case QRTR_TYPE_DATA:
          CommandHandler(qrtr_pkt);
          break;
      }
    }
  }
  DLOGI("Exiting qrtr event thread");
}

SDMCompDisplayType SDMCompService::GetSDMCompDisplayType(DisplayType disp_type) {
  switch(disp_type) {
    case kDisplayTypePrimary:
      return kSDMCompDisplayTypePrimary;
    case kDisplayTypeSecondary1:
      return kSDMCompDisplayTypeSecondary1;
    default:
      return kSDMCompDisplayTypeMax;
  }
}

void SDMCompService::HandlePendingCommands() {
  std::lock_guard<std::mutex> lock(pending_cmd_lock_);
  Response rsp = {};
  for (auto cmd : pending_commands_) {
    int cmd_id = cmd.first;
    const struct qrtr_packet &qrtr_pkt = cmd.second;
    switch (cmd_id) {
      case kCmdExportDemuraBuffers:
        HandleImportDemuraBuffers(qrtr_pkt, &rsp);
        break;
      case kCmdSetBacklight: {
        HandleSetBacklight(qrtr_pkt, &rsp);
      } break;
      case kCmdSetDisplayConfig: {
        HandleSetDisplayConfigs(qrtr_pkt, &rsp);
      } break;
      case kCmdSetProperties: {
        HandleSetProperties(qrtr_pkt, &rsp);
      } break;
      case kCmdSetPanelBootParams: {
        HandleSetPanelBootParams(qrtr_pkt, &rsp);
      } break;
      default:
        break;
    }
    if (qrtr_pkt.data) {
      delete [] qrtr_pkt.data;
    }
  }
  pending_commands_.clear();
}

bool SDMCompService::IsRegisteredClientValid() {
  for (auto callback : callbacks_) {
    if (callback) {
      return true;
    }
  }
  return false;
}

}
