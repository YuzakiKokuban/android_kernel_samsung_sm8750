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

#ifndef __SDM_COMP_IMPL_H__
#define __SDM_COMP_IMPL_H__

#include <core/core_interface.h>
#include <errno.h>

#include <mutex>
#include <condition_variable>

#include "sdm_comp_buffer_sync_handler.h"
#include "sdm_comp_buffer_allocator.h"
#include "sdm_comp_interface.h"
#include "core/display_interface.h"
#include "sdm_comp_service_intf.h"
#include "sdm_comp_display_builtin.h"

namespace sdm {

using std::recursive_mutex;
using std::lock_guard;

class SDMCompIPCImpl : public IPCIntf {
public:
  int Init() { return 0; }
  int Deinit();
  int SetParameter(IPCParams param, const GenericPayload &in);
  int GetParameter(IPCParams param, GenericPayload *out);
  int ProcessOps(IPCOps op, const GenericPayload &in, GenericPayload *out);

private:
  SDMCompServiceDemuraBufInfo hfc_buf_info_ {};
};

class SDMCompImpl : public SDMCompInterface, SDMCompServiceCbIntf {
 public:
  static SDMCompImpl *GetInstance();
  virtual ~SDMCompImpl() { }

  int Init();
  int Deinit();
  int SetParameter(IPCParams param, const GenericPayload &in);
  int GetParameter(IPCParams param, GenericPayload *out);
  int ProcessOps(IPCOps op, const GenericPayload &in, GenericPayload *out);

 protected:
  virtual int CreateDisplay(SDMCompDisplayType display_type, CallbackInterface *callback,
                            Handle *disp_hnd);
  virtual int DestroyDisplay(Handle disp_hnd);
  virtual int GetDisplayAttributes(Handle disp_hnd, SDMCompDisplayAttributes *display_attributes);
  virtual int ShowBuffer(Handle disp_hnd, BufferHandle *buf_handle, int32_t *retire_fence);
  virtual int SetColorModeWithRenderIntent(Handle disp_hnd, struct ColorMode mode);
  virtual int GetColorModes(Handle disp_hnd, uint32_t *out_num_modes,
                            struct ColorMode *out_modes);
  virtual int SetPanelBrightness(Handle disp_hnd, float brightness_level);
  virtual int SetMinPanelBrightness(Handle disp_hnd, float min_brightness_level);
  virtual int OnEvent(SDMCompServiceEvents event, ...);

  void HandlePendingEvents();

  static SDMCompImpl *sdm_comp_impl_;
  static SDMCompDisplayBuiltIn *display_builtin_[kSDMCompDisplayTypeMax];
  static uint32_t ref_count_;
  static uint32_t disp_ref_count_[kSDMCompDisplayTypeMax];

  CoreInterface *core_intf_ = nullptr;
  SDMCompBufferAllocator buffer_allocator_;
  SDMCompBufferSyncHandler buffer_sync_handler_;
  SDMCompServiceIntf *sdm_comp_service_intf_ = nullptr;
  float panel_brightness_[kSDMCompDisplayTypeMax] = {0.0f};
  SDMCompServiceDispConfigs disp_configs_[kSDMCompDisplayTypeMax] = {};
  std::map<SDMCompServiceEvents, SDMCompDisplayType> pending_events_ = {};
  std::shared_ptr<SDMCompIPCImpl> ipc_intf_;
  bool first_commit_ = true;
  int rc_enabled_ = 0;
};

}  // namespace sdm

#endif  // __SDM_COMP_IMPL_H__

