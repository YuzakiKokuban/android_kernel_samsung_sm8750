/*
 * Copyright (c) 2020-2021, The Linux Foundation. All rights reserved.
 *
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
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

#ifndef __IPC_INTERFACE_H__
#define __IPC_INTERFACE_H__

#include <utils/constants.h>
#include <string>
#include <map>
#include <utility>
#include <vector>

#include "private/generic_intf.h"
#include "private/generic_payload.h"
#include "vm_interface.h"

namespace sdm {

enum IPCParams {
  kIpcParamBacklight,        //!< Send backlight params to SVM
  kIpcParamDisplayConfigs,   //!< Send display config information to SVM
  kIpcParamProperties,       //!< Send display properties to SVM
  kIpcParamSetDemuraBuffer,  //!< Cache hfc buffers inside sdm composer
  kIpcParamPanelBoot,        //!< Send the panel boot parameter to SVM
  kIpcParamSetHFCBuffer,     //!< Send the HFC buffer to SVM
  kIPCParamMax,
};

enum IPCOps {
  kIpcOpsFilePath,
  kIpcOpsExportBuffers,
  kIpcOpsImportBuffers,
  kIpcOpsRegisterVmCallback,
  kIpcOpsUnRegisterVmCallback,
  kIPCOpMax
};

struct IPCBacklightParams {
  float brightness = 0.0f;        //!< Specifies the brightness level to be passed to SVM
  bool is_primary = false;        //!< Flag specifies primary/secondary
};

struct IPCDisplayConfigParams {
  uint32_t h_total = 0;           //!< Total width of panel (hActive + hFP + hBP + hPulseWidth)
  uint32_t v_total = 0;           //!< Total height of panel (vActive + vFP + vBP + vPulseWidth)
  uint32_t fps = 0;               //!< Frame rate per second.
  bool is_primary = false;        //!< Flag specifies primary/secondary
  bool smart_panel = false;       //!< If the display config has smart panel.
  uint32_t mixer_width = 0;
  uint32_t mixer_height = 0;
};

struct DemuraPaths {
  std::string configPath = "";
  std::string signaturePath = "";
  std::string publickeyPath = "";
  DemuraPaths() {}
  ~DemuraPaths() {}
  DemuraPaths(std::string config, std::string sig, std::string pk)
      : configPath(config), signaturePath(sig), publickeyPath(pk) {}
};

struct IPCSetPropertyParams {
  Properties props;
};

struct IPCBufferInfo {
  int fd;
  uint32_t size;
  uint32_t payload_sz;
  uint64_t panel_id;
  char file_name[128];
  int64_t mem_handle;
};

struct IPCPanelBootParams {
  std::string panel_boot_string = "";
};

enum IPCBufferType {
  kIpcBufferTypeDemuraHFC,
  kIpcBufferTypeMax,
};

class IPCVmCallbackIntf {
 public:
  virtual void OnServerReady() = 0;
  virtual void OnServerExit() = 0;
 protected:
  virtual ~IPCVmCallbackIntf() {}
};

struct IPCImportBufInParams {
  IPCBufferType req_buf_type = {};
  uint64_t panel_id = 0;
};

struct IPCImportBufOutParams {
  std::vector<IPCBufferInfo> buffers;
};

struct IPCExportBufInParams {
  std::map<IPCBufferType, IPCBufferInfo> buffers = {};
  uint64_t panel_id = 0;
};

struct IPCExportBufOutParams {
  std::map<IPCBufferType, int> exported_fds = {};
};

using IPCIntf = sdm::GenericIntf<IPCParams, IPCOps, GenericPayload>;

}  // namespace sdm

#endif  // __IPC_INTERFACE_H__
