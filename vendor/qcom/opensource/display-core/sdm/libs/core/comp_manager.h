/*
* Copyright (c) 2014 - 2021, The Linux Foundation. All rights reserved.
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

#ifndef __COMP_MANAGER_H__
#define __COMP_MANAGER_H__

#include <core/display_interface.h>
#include <private/extension_interface.h>
#include <private/hw_interface.h>
#include <private/spr_intf.h>
#include <utils/locker.h>
#include<limits.h>
#include <bitset>
#include <set>
#include <vector>
#include <string>
#include <map>
#include <mutex>

#include "strategy.h"
#include "resource_default.h"

namespace sdm {

class CompManagerEventHandler {
 public:
  virtual ~CompManagerEventHandler() {}
  virtual void NotifyCwbDone(int32_t status, const LayerBuffer& buffer) = 0;
  virtual void Refresh() = 0;
  virtual void OnCwbTeardown(bool sync_teardown) = 0;
};

class CompManager : public CwbCallback {
 public:
  DisplayError Init(const std::vector<HWResourceInfo> &hw_res_info_,
                    ExtensionInterface *extension_intf,
                    BufferAllocator *buffer_allocator, SocketHandler *socket_handler);
  DisplayError Deinit();
  DisplayError RegisterDisplay(DisplayId display_id, SDMDisplayType type,
                               DisplayDeviceContext &device_ctx,
                               DisplayClientContext &client_ctx, Handle *display_ctx,
                               std::map<uint32_t, HWQosData> *default_qos_data,
                               CompManagerEventHandler *event_handler);
  DisplayError UnregisterDisplay(Handle display_ctx);
  DisplayError ReconfigureDisplay(Handle display_ctx, DisplayDeviceContext &device_ctx,
                                  DisplayClientContext &client_ctx,
                                  std::map<uint32_t, HWQosData> *default_qos_data);
  DisplayError PrePrepare(Handle display_ctx, DispLayerStack *disp_layer_stack);
  DisplayError Prepare(Handle display_ctx, DispLayerStack *disp_layer_stack);
  DisplayError Commit(Handle display_ctx, DispLayerStack *disp_layer_stack);
  DisplayError PostPrepare(Handle display_ctx, DispLayerStack *disp_layer_stack);
  DisplayError PostCommit(Handle display_ctx, DispLayerStack *disp_layer_stack);
  void Purge(Handle display_ctx);
  DisplayError SetIdleTimeoutMs(Handle display_ctx, uint32_t active_ms, uint32_t inactive_ms);
  void ProcessIdleTimeout(Handle display_ctx);
  void DoGpuFallback(Handle display_ctx);
  void ProcessThermalEvent(Handle display_ctx, int64_t thermal_level);
  void ProcessIdlePowerCollapse(Handle display_ctx);
  DisplayError SetMaxMixerStages(Handle display_ctx, uint32_t max_mixer_stages);
  void ControlPartialUpdate(Handle display_ctx, bool enable);
  DisplayError ValidateScaling(const LayerRect &crop, const LayerRect &dst, bool rotate90);
  DisplayError ValidateAndSetCursorPosition(Handle display_ctx, DispLayerStack *disp_layer_stack,
                                            int x, int y);
  bool SetDisplayState(Handle display_ctx, DisplayState state, const SyncPoints &sync_points);
  DisplayError SetMaxBandwidthMode(HWBwModes mode);
  DisplayError GetScaleLutConfig(HWScaleLutInfo *lut_info);
  DisplayError SetDetailEnhancerData(Handle display_ctx, const DisplayDetailEnhancerData &de_data);
  DisplayError SetCompositionState(Handle display_ctx, LayerComposition composition_type,
                                   bool enable);
  DisplayError ControlDpps(bool enable);
  DisplayError SetColorModesInfo(Handle display_ctx,
                                 const std::vector<PrimariesTransfer> &colormodes_cs);
  DisplayError SetBlendSpace(Handle display_ctx, const PrimariesTransfer &blend_space);
  void HandleSecureEvent(Handle display_ctx, SecureEvent secure_event);
  void PostHandleSecureEvent(Handle display_ctx, SecureEvent secure_event);
  void SetSafeMode(bool enable);
  bool IsSafeMode();
  void GenerateROI(Handle display_ctx, DispLayerStack *disp_layer_stack);
  DisplayError CheckEnforceSplit(Handle comp_handle, uint32_t new_refresh_rate);
  DppsControlInterface* GetDppsControlIntf();
  bool CheckResourceState(Handle display_ctx, bool *res_exhausted, HWDisplayAttributes attr);
  bool IsRotatorSupportedFormat(LayerBufferFormat format);
  DisplayError SetDrawMethod(Handle display_ctx, const DisplayDrawMethod &draw_method);
  DisplayError FreeDemuraFetchResources(const uint32_t &display_id);
  DisplayError GetDemuraFetchResourceCount(MultiDpuDemuraMap *fetch_resource_cnt);
  DisplayError ReserveDemuraFetchResources(const uint32_t &display_id,
                                           const int8_t &preferred_rect);
  DisplayError GetDemuraFetchResources(Handle display_ctx, std::vector<FetchResourceList> *frl);
  DisplayError ReserveABCFetchResources(const uint32_t &display_id, bool is_primary,
                                        const int8_t &req_cnt);
  void SetDemuraStatus(bool status);
  bool GetDemuraStatus();
  void SetDemuraStatusForDisplay(const int32_t &display_id, bool status);
  bool GetDemuraStatusForDisplay(const int32_t &display_id);
  DisplayError SetMaxSDEClk(Handle display_ctx, uint32_t clk);
  void GetRetireFence(Handle display_ctx, shared_ptr<Fence> *retire_fence);
  void NeedsValidate(Handle display_ctx, bool *needs_validate);
  DisplayError SetBacklightLevel(Handle display_ctx, const uint32_t &backlight_level);
  DisplayError GetHDRCapability(bool *hdr_plus_support, bool *dolby_vision_supported);
  DisplayError ForceToneMapConfigure(Handle display_ctx, DispLayerStack *disp_layer_stack);
  DisplayError GetDefaultQosData(Handle display_ctx,
                                 std::map<uint32_t, HWQosData> *default_qos_data);
  DisplayError HandleCwbFrequencyBoost(bool isRequest);
  DisplayError PreCommit(Handle display_ctx);
  DisplayError CaptureCwb(Handle display_ctx, const LayerBuffer &buffer, const CwbConfig &config);
  bool HasPendingCwbRequest(Handle display_ctx);
  bool HandleCwbTeardown(Handle display_ctx);
  DisplayError RequestVirtualDisplayId(int32_t *vdisp_id);
  DisplayError AllocateVirtualDisplayId(int32_t *vdisp_id);
  DisplayError DeallocateVirtualDisplayId(int32_t vdisp_id);
  virtual void NotifyCwbDone(int32_t display_id, int32_t status, const LayerBuffer& buffer);
  virtual void TriggerRefresh(int32_t display_id);
  virtual void TriggerCwbTeardown(int32_t display_id, bool sync_teardown);
  std::string Dump(Handle display_ctx);
  uint32_t GetMixerCount(DisplayId display_id);
  uint32_t GetActiveDisplayCount();
  void SetDisplayLayerStack(Handle display_ctx, DispLayerStack *disp_layer_stack);
  void GetDSConfig(Handle display_ctx, HWLayersInfo *hw_layers_info);
  bool IsDisplayHWAvailable();
  DisplayError SetSprIntf(Handle display_ctx, std::shared_ptr<SPRIntf> intf);
  bool IsMirroredOfAnyDisplay(int32_t display_id, const LayerStack *layer_stack,
                              int32_t *out_src_display);
  bool IsActiveDisplay(int32_t display_id);

 private:
  static const int kMaxThermalLevel = 3;
  static const int kSafeModeThreshold = 4;

  void PrepareStrategyConstraints(Handle display_ctx, DispLayerStack *disp_layer_stack);
  void UpdateStrategyConstraints(bool is_primary, bool disabled);
  DisplayError HandleQosValidation(Handle display_ctx,
                                   DispLayerStack *disp_layer_stack, DisplayError error);
  std::string StringDisplayList(const std::set<int32_t> &displays);

  struct DisplayCompositionContext {
    Strategy *strategy = NULL;
    StrategyConstraints constraints;
    Handle display_resource_ctx = NULL;
    DisplayId display_id = {};
    SDMDisplayType display_type = kBuiltIn;
    uint32_t max_strategies = 0;
    uint32_t remaining_strategies = 0;
    bool idle_fallback = false;
    // Using primary panel flag of hw panel to configure Constraints. We do not need other hw
    // panel parameters for now.
    bool is_primary_panel = false;
    PUConstraints pu_constraints = {};
    DisplayConfigVariableInfo fb_config = {};
    bool first_cycle_ = true;
    uint32_t dest_scaler_blocks_used = 0;
  };

  std::recursive_mutex comp_mgr_mutex_;
  ResourceInterface *resource_intf_ = NULL;
  std::map<int32_t, CompManagerEventHandler*> callback_map_;
  std::set<int32_t> registered_displays_;  // List of registered displays
  std::set<int32_t> configured_displays_;  // List of sucessfully configured displays
  std::set<int32_t> powered_on_displays_;  // List of powered on displays.
  bool safe_mode_ = false;              // Flag to notify all displays to be in resource crunch
                                        // mode, where strategy manager chooses the best strategy
                                        // that uses optimal number of pipes for each display
  std::vector<HWResourceInfo> hw_res_info_;
  BufferAllocator *buffer_allocator_ = NULL;
  ExtensionInterface *extension_intf_ = NULL;
  CapabilitiesInterface *cap_intf_ = nullptr;
  CwbManagerInterface *cwb_mgr_intf_ = nullptr;
  uint32_t max_sde_secondary_fetch_layers_ = 2;
  uint32_t max_sde_builtin_fetch_layers_ = 2;
  DppsControlInterface *dpps_ctrl_intf_ = NULL;
  bool demura_enabled_ = false;
  std::map<int32_t /* display_id */, bool> display_demura_status_;
  SecureEvent secure_event_ = kSecureEventMax;
};

}  // namespace sdm

#endif  // __COMP_MANAGER_H__
