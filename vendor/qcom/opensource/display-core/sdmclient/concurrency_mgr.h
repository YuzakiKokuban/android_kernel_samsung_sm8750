/*
 * Copyright (c) 2014-2021, The Linux Foundation. All rights reserved.
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
 *    * Neither the name of The Linux Foundation. nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
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
 * Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 * Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#ifndef __CONCURRENCY_MGR_H__
#define __CONCURRENCY_MGR_H__

#include <core/buffer_sync_handler.h>
#include <core/core_interface.h>
#include <core/display_interface.h>
#include <core/ipc_interface.h>
#include <core/socket_handler.h>
#include <utils/constants.h>
#include <utils/locker.h>

#include <atomic>
#include <future>  // NOLINT
#include <map>
#include <memory>
#include <queue>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "display_event_handler.h"
#include "sdm_cwb.h"
#include "sdm_cwb_cb_intf.h"
#include "sdm_display.h"
#include "sdm_display_builder.h"
#include "sdm_display_builder_cb_intf.h"
#include "sdm_display_builtin.h"
#include "sdm_display_intf_aiqe.h"
#include "sdm_display_intf_caps.h"
#include "sdm_display_intf_drawcycle.h"
#include "sdm_display_intf_lifecycle.h"
#include "sdm_display_intf_parcel.h"
#include "sdm_display_intf_settings.h"
#include "sdm_display_intf_sideband.h"
#include "sdm_display_pluggable.h"
#include "sdm_display_pluggable_test.h"
#include "sdm_display_virtual.h"
#include "sdm_display_virtual_factory.h"
#include "sdm_hotplug.h"
#include "sdm_hotplug_cb_intf.h"
#include "sdm_layers.h"
#include "sdm_services.h"
#include "sdm_services_cb_intf.h"
#include "sdm_tui.h"
#include "sdm_tui_cb_intf.h"

namespace sdm {

void GetColorMetadataFromColorMode(SDMColorMode mode, Dataspace &ds);

class ConcurrencyMgr : public SDMDisplaySideBandIntf,
                       public SDMDisplayCapsIntf,
                       public SDMDisplaySettingsIntf,
                       public SDMDisplayLifeCycleIntf,
                       public SDMDisplayDrawCycleIntf,
                       public SDMTrustedUICbIntf,
                       public SDMServicesCbIntf,
                       public SDMHotPlugCbIntf,
                       public SDMConcurrentWriteBackCbIntf,
                       public SDMDisplayBuilderCbIntf,
                       public SDMDisplayEventHandler,
                       public SDMDisplayAiqeIntf {
 public:
  std::unordered_map<Display, SDMDisplay *> sdm_display_{};

  DisplayError GetDisplaysStatus(HWDisplaysInfo *info);
  DisplayError GetMaxDisplaysSupported(SDMDisplayType in_type,
                                       int32_t *max_disp);

  SDMDisplay *GetDisplayFromClientId(Display id) {
    if (sdm_display_.find(id) != sdm_display_.end()) {
      return sdm_display_[id];
    }

    return nullptr;
  }

  void SetDisplayByClientId(Display id, SDMDisplay *disp) {
    if (!disp) {
      pending_power_mode_[id] = false;
    }

    sdm_display_[id] = disp;
  }

  int GetDisplayIndex(int dpy);
  DisplayError CreatePrimaryDisplay();

  ConcurrencyMgr();
  ~ConcurrencyMgr();
  DisplayError Init(BufferAllocator *buffer_allocator, SocketHandler *socket_handler,
                    DebugCallbackIntf *debug) override;
  DisplayError Deinit();
  void RegisterCompositorCallback(SDMCompositorCbIntf *cb, bool enable);

  DisplayError AcceptDisplayChanges(Display display_id);

  bool GetComposerStatus() override;

  void CompositorSync(CompositorSyncType sync_type) override;

  DisplayError PostBuffer(const CwbConfig &cwb_config, void *buffer,
                          int32_t display_type);

  template <typename... Args>
  DisplayError CallDisplayFunction(Display display,
                                   DisplayError (SDMDisplay::*member)(Args...),
                                   Args... args) {
    if (display >= kNumDisplays) {
      return kErrorParameters;
    }

    SCOPE_LOCK(locker_[display]);
    auto status = kErrorParameters;
    if (sdm_display_[display]) {
      auto sdm_display = sdm_display_[display];
      status = (sdm_display->*member)(std::forward<Args>(args)...);
    }
    return status;
  }

#ifdef SEC_GC_QC_OPTIMIZATION
  template <typename... Args>
  DisplayError CallGetDisplayFunction(Display display,
                                   DisplayError (SDMDisplay::*member)(Args...),
                                   Args... args) {
    if (display >= kNumDisplays) {
      return kErrorParameters;
    }

    // SCOPE_LOCK(locker_[display]);
    auto status = kErrorParameters;
    if (sdm_display_[display]) {
      auto sdm_display = sdm_display_[display];
      status = (sdm_display->*member)(std::forward<Args>(args)...);
    }
    return status;
  }
#endif

  void SetHpdData(int hpd_bpp, int hpd_pattern, int hpd_connected) override;
  void GetHpdData(int *hpd_bpp, int *hpd_pattern, int *hpd_connected) override;

  DisplayError
  GetAllDisplayAttributes(uint64_t display_id,
                          std::map<uint32_t, DisplayConfigVariableInfo> *info);

  DisplayError GetDisplayAttributes(uint64_t in_display_id, int32_t in_index,
                                    DisplayConfigVariableInfo *ret);

  DisplayError GetPanelBlMaxLvl(uint64_t in_display_id, int32_t *ret) {
    return kErrorNone; // TODO(user)
  }

  DisplayError GetPanelMaxBrightness(uint64_t in_display_id, int32_t *ret) {
    return kErrorNone; // TODO(user)
  }

  DisplayError GetRefreshRateRange(uint64_t in_display_id, uint32_t *min,
                                   uint32_t *max) {
    return kErrorNone; // TODO(user)
  }

  DisplayError IsUnderScanSupported(uint64_t in_display_id, bool *ret) {
    return kErrorNone; // TODO(user)
  }

  DisplayError GetColorModeAttr(uint64_t in_display_id,
                                const std::string &in_color_mode,
                                std::vector<ColorModeAttributeVal> *ret) {
    return kErrorNone; // TODO(user)
  }

  DisplayError GetDefaultColorMode(uint64_t in_display_id, std::string *ret) {
    return kErrorNone; // TODO(user)
  }

  DisplayError GetDisplayPort(uint64_t in_display_id, DisplayPort *ret) {
    return kErrorNone; // TODO(user)
  }

  DisplayError GetSupportedDSIClock(uint64_t disp_id,
                                    std::vector<int64_t> *bit_clks) override;

  DisplayError GetStcColorModes(uint64_t in_display_id) {
    return kErrorNone; // TODO(user)
  }

  DisplayError IsSupportedOnDisplay(uint64_t in_display_id,
                                    SupportedDisplayFeature in_feature,
                                    bool *ret) {
    return kErrorNone; // TODO(user)
  }

  DisplayError GetDisplayConnectionType(uint64_t display_id,
                                        DisplayClass *display_class);

  DisplayError GetFixedConfig(uint64_t display_id,
                              DisplayConfigFixedInfo *info) {
    return CallDisplayFunction(display_id, &SDMDisplay::GetFixedConfig, info);
  }

  /* Display Settings APIs */
  DisplayError SetBacklightScale(uint64_t display_id, int32_t level) {
    return kErrorNone; // TODO(user)
  }

  DisplayError SetBlendSpace(uint64_t display_id,
                             const PrimariesTransfer &blend_space) {
    return kErrorNone; // TODO(user)
  }

  DisplayError SetVSyncState(uint64_t display_id, bool enabled) {
    return kErrorNone; // TODO(user)
  }

  DisplayError SetAutoRefresh(uint64_t display_id, bool enable) {
    return kErrorNone; // TODO(user)
  }

  DisplayError SetColorMode(uint64_t display_id,
                            const std::string &color_mode) {
    return kErrorNone; // TODO(user)
  }

  DisplayError SetVariableVSyncMode(uint64_t display_id,
                                    const VariableVSync &vsync_mode) {
    return kErrorNone; // TODO(user)
  }

  DisplayError SetJitterConfig(uint64_t display_id, int32_t jitter_type,
                               float value, int32_t time) {
    return kErrorNone; // TODO(user)
  }

  DisplayError SetPanelLuminanceAttributes(uint64_t display_id, float mlum,
                                           float max_lum);

  DisplayError SetExtendedColorMode(uint64_t display_id,
                                    const ColorModeInfo &color_mode) {
    return kErrorNone; // TODO(user)
  }

  DisplayError SetDimmingEnable(uint64_t display_id, int32_t enabled);

  DisplayError SetDimmingMinBacklight(uint64_t display_id, int32_t mbl) {
    return kErrorNone; // TODO(user)
  }

  DisplayError GetPanelBrightness(uint64_t display_id, float *brightness) {
    return kErrorNone; // TODO(user)
  }

  DisplayError SetPanelBrightness(uint64_t display_id, float brightness) {
    return kErrorNone; // TODO(user)
  }

  DisplayError GetRefreshRate(uint64_t display_id, int32_t *refresh_rate) {
    return kErrorNone; // TODO(user)
  }

  DisplayError SetRefreshRate(uint64_t display_id, int32_t refresh_rate,
                              bool final_rate, bool idle_screen) {
    return kErrorNone; // TODO(user)
  }

  DisplayError GetActiveConfig(uint64_t display_id,
                               Config *active_config) override;

  DisplayError GetVSyncState(uint64_t display_id, bool *vsync_state) {
    return kErrorNone; // TODO(user)
  }

  DisplayError CreateDisplay(SDMDisplayType type, int32_t width, int32_t height,
                             int32_t *format, uint64_t *display_id);

  DisplayError DestroyDisplay(uint64_t display_id) { return DestroyVirtualDisplay(display_id); }

  DisplayError GetDisplayList(std::vector<SDMDisplayInfo> *display_info_list) {
    return kErrorNone; // TODO(user)
  }

  DisplayError SetDisplayState(uint64_t display_id, DisplayState state,
                               bool teardown, Fence *fence) {
    return kErrorNone; // TODO(user)
  }

  DisplayError PresentDisplay(uint64_t display_id,
                              DisplayDrawMethod draw_method,
                              LayerStack *layer_stack,
                              SDMPresentResult *present_type) {
    return kErrorNone; // TODO(user)
  }

  DisplayError Flush(uint64_t display_id, LayerStack *layer_stack) {
    return kErrorNone; // TODO(user)
  }

  DisplayError SetCompositionState(uint64_t display_id,
                                   LayerComposition composition, bool enable) {
    return kErrorNone; // TODO(user)
  }

  DisplayError SetExpectedPresentTime(uint64_t display_id,
                                      int64_t expected_time) {
    return kErrorNone; // TODO(user)
  }

  DisplayError GetClientTargetSupport(uint64_t display_id, int32_t in_width,
                                      int32_t in_height,
                                      LayerBufferFormat format,
                                      Dataspace color_metadata);

  DisplayError GetReleaseFences(uint64_t display_id,
                                std::vector<Fence> *fences) {
    return kErrorNone; // TODO(user)
  }

  void LayerStackUpdated(uint64_t display) override {
    if (display >= kNumDisplays) {
      return;
    }

    SCOPE_LOCK(locker_[display]);
    if (sdm_display_[display]) {
      auto sdm_display = sdm_display_[display];
      sdm_display->LayerStackUpdated();
    }
  }

  void WaitForDrawCycleToComplete(uint64_t display) override {
    if (display >= kNumDisplays) {
      return;
    }

    SCOPE_LOCK(locker_[display]);
    if (sdm_display_[display]) {
      auto sdm_display = sdm_display_[display];
      sdm_display->WaitForDrawCycleToComplete();
    }
  }

  int GetProperty(const char *property_name, char *value) override;
  int GetProperty(const char *property_name, int *value) override;

  bool IsHDRDisplay(uint64_t display);
  bool IsDisplayConnected(uint64_t display_id) override;
  DisplayError ConfigureDynRefreshRate(SDMBuiltInDisplayOps ops,
                                       int refresh_rate) override;
  DisplayError SetDisplayAnimating(uint64_t display_id,
                                   bool animating) override;
  void UpdateVSyncSourceOnPowerModeOff() override;
  void UpdateVSyncSourceOnPowerModeDoze() override;
  void SetClientUp() override;
  bool IsBuiltInDisplay(uint64_t disp_id) override;
  bool IsAsyncVDSCreationSupported() override;
  DisplayError CreateVirtualDisplay(int width, int height, int format) override;
  DisplayError GetDSIClk(uint64_t disp_id, uint64_t *bit_clk) override;
  DisplayError SetDSIClk(uint64_t disp_id, uint64_t bit_clk) override;
  DisplayError SetQsyncMode(uint64_t disp_id, QSyncMode mode) override;
  DisplayError IsSmartPanelConfig(uint64_t display_id, uint32_t config_id,
                                  bool *is_smart) override;
  bool IsRotatorSupportedFormat(LayerBufferFormat format) override;
  DisplayError GetDisplayHwId(uint64_t disp_id, int32_t *disp_hw_id) override;
  bool IsModeSwitchAllowed(uint64_t disp_id, int32_t config) override;
  DisplayError GetActiveBuiltinDisplay(uint64_t *disp_id) override;

  void RegisterSideBandCallback(SDMSideBandCompositorCbIntf *cb, bool enable) override;

  void GetCapabilities(uint32_t *outCount, int32_t *outCapabilities);
  void Dump(uint32_t *out_size, char *out_buffer);

  DisplayError CreateVirtualDisplay(uint32_t width, uint32_t height,
                                    int32_t *format, Display *out_display_id);

  DisplayError DestroyVirtualDisplay(Display display);
  DisplayError PresentDisplay(Display display,
                              shared_ptr<Fence> *out_retire_fence);
  DisplayError SetOutputBuffer(uint64_t display, const SnapHandle *buffer,
                               const shared_ptr<Fence> &release_fence);
  DisplayError SetPowerMode(uint64_t display, int32_t int_mode) override;
  DisplayError SetColorMode(Display display, int32_t /*ColorMode*/ int_mode);
  DisplayError
  SetColorModeWithRenderIntent(uint64_t display, int32_t /*ColorMode*/ int_mode,
                               int32_t /*RenderIntent*/ int_render_intent);
  DisplayError SetColorTransform(uint64_t display,
                                 const std::vector<float> &matrix);
  DisplayError getDisplayDecorationSupport(Display display, uint32_t *format,
                                           uint32_t *alpha);
  DisplayError GetReadbackBufferAttributes(Display display, int32_t *format,
                                           int32_t *dataspace);
  DisplayError SetReadbackBuffer(uint64_t display, void *buffer,
                                 const shared_ptr<Fence> &acquire_fence);
  DisplayError GetReadbackBufferFence(uint64_t display,
                                      shared_ptr<Fence> *release_fence);
  uint32_t GetMaxVirtualDisplayCount();
  DisplayError GetDisplayIdentificationData(Display display, uint8_t *outPort,
                                            uint32_t *outDataSize,
                                            uint8_t *outData);
  DisplayError
  GetDisplayCapabilities(Display display,
                         vector<SDMDisplayCapability> *capabilities);
  DisplayError GetDisplayBrightnessSupport(Display display, bool *outSupport);
  DisplayError SetDisplayBrightness(Display display, float brightness);
  DisplayError WaitForResources(bool wait_for_resources,
                                Display active_builtin_id,
                                Display display_id) override;
  DisplayError HandleCwbCallBack(int display_index, void *buffer,
                                 const CwbConfig &cwb_cfg) override;
  void NotifyCWBStatus(int32_t status, void *buffer) override;

  // newly added
  DisplayError GetDisplayType(uint64_t display, int32_t *out_type);
  DisplayError GetColorModes(uint64_t display, uint32_t *out_num_modes,
                             int32_t /*ColorMode*/ *int_out_modes);
  DisplayError GetRenderIntents(Display display, int32_t /*ColorMode*/ int_mode,
                                uint32_t *out_num_intents,
                                int32_t /*RenderIntent*/ *int_out_intents);
  DisplayError GetHdrCapabilities(Display display, uint32_t *out_num_types,
                                  int32_t *out_types, float *out_max_luminance,
                                  float *out_max_average_luminance,
                                  float *out_min_luminance);
  DisplayError GetDisplayName(Display display, uint32_t *out_size,
                              char *out_name);
  DisplayError SetActiveConfig(Display display, int32_t config);
  DisplayError GetChangedCompositionTypes(Display display,
                                          uint32_t *out_num_elements,
                                          LayerId *out_layers,
                                          int32_t *out_types);
  DisplayError GetDisplayRequests(Display display,
                                  int32_t *out_display_requests,
                                  uint32_t *out_num_elements,
                                  LayerId *out_layers,
                                  int32_t *out_layer_requests);
  DisplayError GetReleaseFences(Display display, uint32_t *out_num_elements,
                                LayerId *out_layers,
                                std::vector<shared_ptr<Fence>> *out_fences);
  DisplayError SetClientTarget(uint64_t display, const SnapHandle *target,
                               shared_ptr<Fence> acquire_fence,
                               int32_t dataspace, const SDMRegion &region,
                               uint32_t version);
  DisplayError SetCursorPosition(Display display, LayerId layer, int32_t x,
                                 int32_t y);
  DisplayError GetDataspaceSaturationMatrix(int32_t /*Dataspace*/ int_dataspace,
                                            float *out_matrix);
  DisplayError SetDimmingMinBl(Display display, int32_t min_bl);
  DisplayError
  GetClientTargetProperty(Display display,
                          SDMClientTargetProperty *outClientTargetProperty);
  DisplayError SetDemuraState(Display display, int32_t state);
  DisplayError SetDemuraConfig(Display display, int32_t demura_idx);

  DisplayError SetDisplayedContentSamplingEnabled(Display display, bool enabled,
                                                  uint8_t component_mask,
                                                  uint64_t max_frames);
  DisplayError
  GetDisplayedContentSamplingAttributes(Display display, int32_t *format,
                                        int32_t *dataspace,
                                        uint8_t *supported_components);
  DisplayError GetDisplayedContentSample(
      Display display, uint64_t max_frames, uint64_t timestamp,
      uint64_t *numFrames, int32_t samples_size[NUM_HISTOGRAM_COLOR_COMPONENTS],
      uint64_t *samples[NUM_HISTOGRAM_COLOR_COMPONENTS]);
  DisplayError SetDisplayElapseTime(Display display, uint64_t time);

  DisplayError SetCameraSmoothInfo(SDMCameraSmoothOp op, int32_t fps) override;
  DisplayError NotifyTUIDone(int ret, int disp_id,
                             SDMTUIEventType event_type) override;
  DisplayError SetContentFps(const std::string &name, int32_t fps) override;
  int GetDisplayConfigGroup(uint64_t display, DisplayConfigGroupInfo variable_config);

  // SDMDisplayEventHandler
  virtual void DisplayPowerReset();
  virtual void PerformDisplayPowerReset();
  virtual void PerformQsyncCallback(Display display, bool qsync_enabled,
                                    uint32_t refresh_rate,
                                    uint32_t qsync_refresh_rate);
  virtual void VmReleaseDone(Display display);
  virtual DisplayError NotifyCwbDone(int dpy_index, int32_t status,
                                     uint64_t handle_id);
  virtual int NotifyIdleStatus(bool idle_status);

  DisplayError SetVsyncEnabled(uint64_t display, bool enabled);
  DisplayError GetDozeSupport(Display display, int32_t *out_support);
  DisplayError GetDisplayConfigs(Display display,
                                 std::vector<int32_t> *out_configs);
  DisplayError GetVsyncPeriod(Display disp, uint32_t *vsync_period);
  void Refresh(uint64_t display);

  DisplayError GetDisplayVsyncPeriod(Display display,
                                     VsyncPeriodNanos *out_vsync_period);
  DisplayError SetActiveConfigWithConstraints(
      Display display, Config config,
      const SDMVsyncPeriodChangeConstraints *vsync_period_change_constraints,
      SDMVsyncPeriodChangeTimeline *out_timeline);
  DisplayError CommitOrPrepare(Display display, bool validate_only,
                               shared_ptr<Fence> *out_retire_fence,
                               uint32_t *out_num_types,
                               uint32_t *out_num_requests, bool *needs_commit);
  DisplayError TryDrawMethod(Display display, DisplayDrawMethod drawMethod);
  DisplayError SetExpectedPresentTime(Display display,
                                      uint64_t expectedPresentTime);
  DisplayError GetDisplayBrightness(uint64_t display, float *brightness);
  virtual DisplayError NotifyCallback(uint32_t command, SDMParcel *input_parcel,
                                      SDMParcel *output_parcel);

  bool IsClientConnected() { return client_connected_; }
  Display GetVsyncSource() override { return vsync_source_; }
  bool VsyncCallbackRegistered() override { return client_connected_; }
  DisplayError SetupVRRConfig(uint64_t display_id);
  DisplayError NotifyExpectedPresent(Display display, uint64_t expected_present_time,
                                     uint32_t frame_interval_ns);
  DisplayError SetFrameIntervalNs(Display display, uint32_t frameIntervalNs);
  int GetNotifyEptConfig(Display display);
  std::mutex *GetLumMutex() { return &mutex_lum_; }

  DisplayError SetSsrcMode(uint64_t display_id, const std::string &mode_name);
  DisplayError EnableCopr(uint64_t display_id, bool enable);
  DisplayError GetCoprStats(uint64_t display_id, std::vector<int32_t> *copr_stats);
  DisplayError SetABCState(uint64_t display_id, bool state);
  DisplayError SetABCReconfig(uint64_t display_id);
  DisplayError SetABCMode(uint64_t display_id, string mode_name);
  DisplayError SetPanelFeatureConfig(Display display, int32_t type, void *data);

  static const int locker_count_ = pluggable_lock_index_ + 1;
  static Locker locker_[locker_count_];
  static Locker display_config_locker_;
  static std::bitset<kClientMax> clients_waiting_for_commit_[kNumDisplays];
  static shared_ptr<Fence> retire_fence_[kNumDisplays];
  static int commit_error_[kNumDisplays];

private:
  static const int kExternalConnectionTimeoutMs = 500;
  static const int kCommitDoneTimeoutMs = 100;
  static const int kDenomNstoMs = 1000000;
  static const int kNumDrawCycles = 3;
  uint32_t throttling_refresh_rate_ = 60;
  std::mutex hotplug_mutex_;
  std::condition_variable hotplug_cv_;
  bool resource_ready_ = false;
  Display active_display_id_ = 0;
  shared_ptr<Fence> cached_retire_fence_ = nullptr;
  void UpdateThrottlingRate();
  void SetNewThrottlingRate(uint32_t new_rate);

  void ResetPanel();
  DisplayError InitSubModules(DebugCallbackIntf *debug);

  void SendHotplug(Display display, bool state);
  DisplayError Hotplug(Display display, bool state);
  void SendRefresh(Display display);

  DisplayError GetConfigCount(uint64_t disp_id, uint32_t *count);
  DisplayError GetActiveConfigIndex(uint64_t disp_id, uint32_t *config);
  DisplayError SetActiveConfigIndex(uint64_t disp_id, uint32_t config);
  DisplayError SetNoisePlugInOverride(uint64_t disp_id, bool override_en,
                                      int32_t attn, int32_t noise_zpos);
  DisplayError ControlPartialUpdate(uint64_t disp_id, bool enable);
  DisplayError DisplayBWTransactionPending(bool *status);
  DisplayError SetDisplayStatus(uint64_t disp_id, SDMDisplayStatus status);
  DisplayError MinHdcpEncryptionLevelChanged(uint64_t disp_id,
                                             uint32_t min_enc_level);
  DisplayError IsWbUbwcSupported(bool *value);
  DisplayError SetIdleTimeout(uint32_t value);
  DisplayError ToggleScreenUpdate(bool on);
  DisplayError SetCameraLaunchStatus(uint32_t on);
  DisplayError SetDisplayDppsAdROI(uint64_t display_id, uint32_t h_start,
                                   uint32_t h_end, uint32_t v_start,
                                   uint32_t v_end, uint32_t factor_in,
                                   uint32_t factor_out);
  DisplayError ControlIdlePowerCollapse(bool enable, bool synchronous);
  DisplayError GetSupportedDisplayRefreshRates(
      int disp_id, std::vector<uint32_t> *supported_refresh_rates);
  DisplayError SetDynamicDSIClock(int64_t disp_id, uint32_t bitrate);
  DisplayError getDisplayMaxBrightness(uint32_t display,
                                       uint32_t *max_brightness_level);
  void PostInit();

#ifdef PROFILE_COVERAGE_DATA
  DisplayError DumpCodeCoverage(const SDMParcel *input_parcel);
#endif

  // QClient methods
  DisplayError GetDisplayPortId(uint32_t disp_id, int *port_id);
  DisplayError IsCacV2Supported(uint32_t disp_id, bool *supported) {
    return CallDisplayFunction(disp_id, &SDMDisplay::IsCacV2Supported, supported);
  }
  DisplayError PerformCacConfig(uint64_t disp_id, CacConfig cac_config, bool enable) {
    return CallDisplayFunction(disp_id, &SDMDisplay::PerformCacConfig, cac_config, enable);
  }
  // Internal methods
  void HandleSecureSession();
  void HandlePendingPowerMode(Display display,
                              const shared_ptr<Fence> &retire_fence);
  void HandlePendingHotplug(Display disp_id,
                            const shared_ptr<Fence> &retire_fence);
  bool IsPluggableDisplayConnected();
  bool IsVirtualDisplayConnected();
  void HandlePendingRefresh();
  void NotifyClientStatus(bool connected);
  DisplayError TUITransitionPrepare(int disp_id);
  DisplayError TUITransitionStart(int disp_id);
  DisplayError TUITransitionEnd(int disp_id);
  DisplayError TUITransitionEndLocked(int disp_id);
  DisplayError TUITransitionUnPrepare(int disp_id);
  void PerformIdleStatusCallback(Display display);
  DisplayError TeardownConcurrentWriteback(Display display);
  void PostCommitUnlocked(Display display,
                          const shared_ptr<Fence> &retire_fence);
  void PostCommitLocked(Display display, shared_ptr<Fence> &retire_fence);
  DisplayError WaitForCommitDone(Display display, int client_id);
  DisplayError WaitForCommitDoneAsync(Display display, int client_id);
  void NotifyDisplayAttributes(Display display, Config config) override;
  DisplayError WaitForVmRelease(Display display, int timeout_ms);
  void RemoveDisconnectedPluggableDisplays();
  void HpdEventHandler() override;

  DisplayError HandleTUITransition(int disp_id, int event);
  DisplayError TUIEventHandler(uint64_t disp_id, SDMTUIEventType event_type);

  CoreInterface *core_intf_ = nullptr;
  SDMCompositorCallbacks callbacks_{};
  BufferAllocator *buffer_allocator_ = nullptr;

  bool update_vsync_on_power_off_ = false;
  bool update_vsync_on_doze_ = false;
  std::map<Display, Display> map_sdm_display_; // Real and dummy display pairs.
  bool reset_panel_ = false;
  bool client_connected_ = false;
  bool new_bw_mode_ = false;
  SocketHandler *socket_handler_ = nullptr;
  bool hdmi_is_primary_ = false;
  bool is_composer_up_ = false;
  std::mutex mutex_lum_;
  static bool pending_power_mode_[kNumDisplays];

  int32_t idle_pc_ref_cnt_ = 0;

  int32_t enable_primary_reconfig_req_ = 0;
  Display vsync_source_ =
      SDM_DISPLAY_PRIMARY; // hw vsync is active on this display
  std::bitset<kNumDisplays>
      pending_refresh_; // Displays waiting to get refreshed
  std::bitset<kNumDisplays>
      client_pending_refresh_; // compositor refresh pending

  bool async_vds_creation_ = false;
  bool tui_state_transition_[kNumDisplays] = {};
  bool secure_session_active_ = false;
  bool is_client_up_ = false;
  std::shared_ptr<IPCIntf> ipc_intf_ = nullptr;
  Locker primary_display_lock_;
  bool primary_pending_ = true;

  std::map<uint64_t, std::future<DisplayError>> commit_done_future_;
  bool disable_get_screen_decorator_support_ = false;

  SDMHotPlug *hpd_ = nullptr;
  SDMConcurrentWriteBack *cwb_ = nullptr;
  SDMDisplayBuilder *disp_ = nullptr;
  SDMTrustedUI *tui_ = nullptr;

  int hpd_bpp_ = 0;
  int hpd_pattern_ = 0;
  int hpd_connected_ = 0;
  SDMServices *services_ = nullptr;

  std::vector<Display> pending_hotplugs_{};

  // debug callbacks
  // void Refresh(int idx) { callbacks_.Refresh(idx); }
  CoreInterface *GetCoreIntf() { return core_intf_; }
  int &GetIdlePcRefCnt() { return idle_pc_ref_cnt_; }

  static ConcurrencyMgr *cm_;
  static uint32_t cm_ref_count_;
  static std::mutex cm_lock_;

  Locker client_lock_;

  std::shared_ptr<ISnapMapper> snapmapper_ = nullptr;
};
} // namespace sdm

#endif // __SDM_SESSION_H__
