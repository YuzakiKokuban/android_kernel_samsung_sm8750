/*
* Copyright (c) 2014 - 2016, 2018, 2020-2021 The Linux Foundation. All rights reserved.
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
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __CORE_IMPL_H__
#define __CORE_IMPL_H__

#include <core/core_interface.h>
#include <private/extension_interface.h>
#include <private/color_interface.h>
#include <private/panel_feature_factory_intf.h>
#include <private/utils_factory_intf.h>
#include <private/hw_interface.h>
#include <utils/locker.h>
#include <utils/sys.h>
#include <utils/multi_core_instantiator.h>

#include <memory>
#include <vector>
#include <utility>
#include <thread>
#include <mutex>
#include <condition_variable>

#include "comp_manager.h"

#define SET_REVISION(major, minor) ((major << 8) | minor)
#define GET_PANEL_FEATURE_FACTORY "GetPanelFeatureFactoryIntf"

namespace sdm {

typedef PanelFeatureFactoryIntf* (*GetPanelFeatureFactory)();
typedef UtilsFactoryIntf* (*GetUtilsFactory)();

class CoreIPCVmCallbackImpl : public IPCVmCallbackIntf {
 public:
  CoreIPCVmCallbackImpl(std::shared_ptr<IPCIntf> ipc_intf, HWInfoInterface *hw_info_intf);
  void Init();
  void OnServerReady();
  void OnServerExit();
  void Deinit();
  void OnServerReadyThread();
  virtual ~CoreIPCVmCallbackImpl() {}

 private:
  int SendPanelBootParams();
  int cb_hnd_out_ = 0;
  std::shared_ptr<IPCIntf> ipc_intf_ = nullptr;
  HWInfoInterface *hw_info_intf_ = nullptr;
  bool server_ready_ = false;
  bool server_thread_exit_ = false;

  std::thread server_thread_;
  std::mutex server_thread_lock_;
  std::condition_variable server_thread_ready_cv_;
  std::condition_variable server_thread_cv_;
};

class CoreImpl : public CoreInterface {
 public:
  // This class implements display core interface revision 1.0.
  static const uint16_t kRevision = SET_REVISION(1, 0);
  CoreImpl(BufferAllocator *buffer_allocator, SocketHandler *socket_handler,
           std::shared_ptr<IPCIntf> ipc_intf, std::bitset<8> core_ids);
  virtual ~CoreImpl() { }

  // This method returns the interface revision for the current display core object.
  // Future revisions will override this method and return the appropriate revision upon query.
  virtual uint16_t GetRevision() { return kRevision; }
  virtual DisplayError Init();
  virtual DisplayError Deinit();

  // Methods from core interface
  virtual DisplayError CreateDisplay(SDMDisplayType type, DisplayEventHandler *event_handler,
                                     DisplayInterface **intf);
  virtual DisplayError CreateDisplay(int32_t display_id, DisplayEventHandler *event_handler,
                                     DisplayInterface **intf);
  virtual DisplayError CreateNullDisplay(DisplayInterface **intf);
  virtual DisplayError DestroyDisplay(DisplayInterface *intf);
  virtual DisplayError DestroyNullDisplay(DisplayInterface *intf);
  virtual DisplayError SetMaxBandwidthMode(HWBwModes mode);
  virtual DisplayError GetFirstDisplayInterfaceType(HWDisplayInterfaceInfo *hw_disp_info);
  virtual DisplayError GetDisplaysStatus(HWDisplaysInfo *hw_displays_info);
  virtual DisplayError GetMaxDisplaysSupported(SDMDisplayType type, int32_t *max_displays);
  virtual bool IsRotatorSupportedFormat(LayerBufferFormat format);
  virtual DisplayError ReserveDemuraPipeResources();
  virtual DisplayError RequestVirtualDisplayId(int32_t *vdisp_id);
#ifdef PROFILE_COVERAGE_DATA
  virtual DisplayError DumpCodeCoverage();
#endif

 protected:
  void InitializeSDMUtils();
  void ReleaseDemuraResources();
  void OverRideDemuraPanelIds(std::vector<uint64_t> *panel_ids);
  DisplayError CreateNullDisplayLocked(DisplayInterface **intf);
  DisplayError HandleNullDisplay();
  DisplayError ReserveDemuraResources(std::map<uint32_t, uint8_t> required_demura_fetch_cnt);
  DisplayError ReserveABCResources(std::map<uint32_t, uint8_t> required_abc_fetch_cnt);

  Locker locker_;
  BufferAllocator *buffer_allocator_ = NULL;
  std::vector<HWResourceInfo> hw_resource_;
  CompManager comp_mgr_;
  sdm::MultiCoreInstance<uint32_t, HWInfoInterface *> hw_info_intf_;
  DynLib extension_lib_;
  ExtensionInterface *extension_intf_ = NULL;
  CreateExtensionInterface create_extension_intf_ = NULL;
  DestroyExtensionInterface destroy_extension_intf_ = NULL;
  PanelFeatureFactoryIntf *panel_feature_factory_intf_ = NULL;
  UtilsFactoryIntf *sdm_utils_factory_intf_ = NULL;
  SocketHandler *socket_handler_ = NULL;
  HWDisplaysInfo hw_displays_info_ = {};
  std::shared_ptr<IPCIntf> ipc_intf_ = nullptr;
  CoreIPCVmCallbackImpl* vm_cb_intf_ = nullptr;
  std::vector<uint64_t> *panel_ids_;
  std::shared_ptr<DemuraParserManagerIntf> pm_intf_ = nullptr;
  bool reserve_done_ = false;
  char *raw_mapped_buffer_ = nullptr;
  std::vector<uint32_t> demura_display_ids_;
  bool enable_null_display_ = false;
  std::bitset<8> core_ids_ = std::bitset<8>(0xFF);
};

}  // namespace sdm

#endif  // __CORE_IMPL_H__

