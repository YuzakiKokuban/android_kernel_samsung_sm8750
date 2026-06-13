/*
 * Copyright (c) 2014-2021, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
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
#ifndef __SDM_DISPLAY_H__
#define __SDM_DISPLAY_H__

#include "display_event_handler.h"
#include "sdm_layers.h"
#include <algorithm>
#include <bitset>
#include <core/buffer_sync_handler.h>
#include <core/core_interface.h>
#include <map>
#include <private/color_params.h>
#include <queue>
#include <set>
#include <string>
#include <sys/stat.h>
#include <utility>
#include <vector>
#include <climits>

#include "sdm_compositor_callbacks.h"
#include "sdm_layer_builder.h"

namespace sdm {

enum {
  INPUT_LAYER_DUMP,
  OUTPUT_LAYER_DUMP,
};

enum SecureSessionType {
  kSecureDisplay,
  kSecureCamera,
  kSecureTUI,
  kSecureMax,
};

// CWB client currently using the block
enum CWBClient {
  kCWBClientNone,      // No client connected
  kCWBClientFrameDump, // Dump to file
  kCWBClientColor,     // Internal client i.e. Color Manager
  kCWBClientExternal,  // External client calling through private APIs
  kCWBClientComposer,  // Client to SDM i.e. SurfaceFlinger
};

enum CWBReleaseFenceError {
  kCWBReleaseFenceErrorNone,
  kCWBReleaseFenceSignaled = kCWBReleaseFenceErrorNone,
  kCWBReleaseFenceWaitTimedOut,
  kCWBReleaseFenceNotAvailable,
  kCWBReleaseFenceNotChecked,
  kCWBReleaseFencePending,
  kCWBReleaseFenceUnknownError,
};

struct CWBCaptureResponse {
  uint64_t handle_id = 0;
  CWBClient client = kCWBClientNone;
  CWBReleaseFenceError status = kCWBReleaseFenceErrorNone;
  std::shared_ptr<Fence> release_fence = nullptr;
};

struct TransientRefreshRateInfo {
  uint32_t transient_vsync_period;
  int64_t vsync_applied_time;
};

class SDMColorModeMgr {
public:
  SDMColorModeMgr(){};
  explicit SDMColorModeMgr(DisplayInterface *display_intf);
  virtual ~SDMColorModeMgr() {}
  virtual DisplayError Init();
  virtual DisplayError DeInit();
  virtual void Dump(std::ostringstream *os);
  virtual uint32_t GetColorModeCount();
  virtual uint32_t GetRenderIntentCount(SDMColorMode mode);
  virtual DisplayError GetColorModes(uint32_t *out_num_modes,
                                     SDMColorMode *out_modes);
  virtual DisplayError GetRenderIntents(SDMColorMode mode,
                                        uint32_t *out_num_intents,
                                        SDMRenderIntent *out_modes);
  DisplayError SetColorModeWithRenderIntent(SDMColorMode mode,
                                            SDMRenderIntent intent);
  DisplayError SetColorModeById(int32_t color_mode_id);
  DisplayError SetColorModeFromClientApi(std::string mode_string);
  virtual DisplayError SetColorTransform(const float *matrix, SDMColorTransform hint);
  virtual DisplayError RestoreColorTransform();
  virtual SDMColorMode GetCurrentColorMode() { return current_color_mode_; }
  virtual SDMRenderIntent GetCurrentRenderIntent() {
    return current_render_intent_;
  }
  virtual DisplayError ApplyCurrentColorModeWithRenderIntent(bool hdr_present);
  virtual DisplayError CacheColorModeWithRenderIntent(SDMColorMode mode,
                                                      SDMRenderIntent intent);
  void ReapplyMode() { apply_mode_ = true; };
  virtual DisplayError NotifyDisplayCalibrationMode(bool in_calibration) {
    return kErrorNotSupported;
  }

protected:
  template <class T>
  void CopyColorTransformMatrix(const T *input_matrix, double *output_matrix) {
    for (uint32_t i = 0; i < kColorTransformMatrixCount; i++) {
      output_matrix[i] = static_cast<double>(input_matrix[i]);
    }
  }

  static const uint32_t kColorTransformMatrixCount = 16;
  DisplayInterface *display_intf_ = NULL;
  bool apply_mode_ = false;
  SDMColorMode current_color_mode_ = SDMColorMode::COLOR_MODE_NATIVE;
  SDMRenderIntent current_render_intent_ = SDMRenderIntent::COLORIMETRIC;
  DynamicRangeType curr_dynamic_range_ = kSdrType;

  double color_matrix_[kColorTransformMatrixCount] = {
      1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0,
      0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0};

private:
  void PopulateColorModes();
  DisplayError ValidateColorModeWithRenderIntent(SDMColorMode mode,
                                                 SDMRenderIntent intent);
  DisplayError SetPreferredColorModeInternal(const std::string &mode_string,
                                             bool from_client,
                                             SDMColorMode *color_mode,
                                             DynamicRangeType *dynamic_range);

  typedef std::map<DynamicRangeType, std::string> DynamicRangeMap;
  typedef std::map<SDMRenderIntent, DynamicRangeMap> RenderIntentMap;
  // Initialize supported mode/render intent/dynamic range combination
  std::map<SDMColorMode, RenderIntentMap> color_mode_map_ = {};
  std::map<SDMColorMode, DynamicRangeMap> preferred_mode_ = {};
};

class SDMDisplay : public DisplayEventHandler {
public:
  virtual ~SDMDisplay() {}

  virtual DisplayError Init();
  virtual DisplayError Deinit();

  virtual DisplayError GetFixedConfig(DisplayConfigFixedInfo *info);

  // Framebuffer configurations
  virtual void SetIdleTimeoutMs(uint32_t timeout_ms, uint32_t inactive_ms);
  virtual DisplayError SetFrameDumpConfig(uint32_t count,
                                          uint32_t bit_mask_layer_type,
                                          int32_t format);
  virtual DisplayError SetFrameDumpConfig(uint32_t count,
                                          uint32_t bit_mask_layer_type,
                                          int32_t format,
                                          CwbConfig &cwb_config);
  virtual DisplayError SetMaxMixerStages(uint32_t max_mixer_stages);
  virtual DisplayError ControlPartialUpdate(bool enable, uint32_t *pending) {
    return kErrorNotSupported;
  }
  virtual SDMPowerMode GetCurrentPowerMode();
  virtual DisplayError SetFrameBufferResolution(uint32_t x_pixels,
                                                uint32_t y_pixels);
  virtual void GetFrameBufferResolution(uint32_t *x_pixels, uint32_t *y_pixels);
  virtual DisplayError SetDisplayStatus(SDMDisplayStatus display_status);
  virtual DisplayError OnMinHdcpEncryptionLevelChange(uint32_t min_enc_level);
  virtual DisplayError Perform(uint32_t operation, ...);
  virtual DisplayError
  HandleSecureSession(const std::bitset<kSecureMax> &secure_sessions,
                      bool *power_on_pending, bool is_active_secure_display);
  virtual DisplayError HandleSecureEvent(SecureEvent secure_event,
                                         bool *needs_refresh,
                                         bool update_event_only);
  virtual DisplayError PostHandleSecureEvent(SecureEvent secure_event);
  virtual DisplayError
  GetActiveSecureSession(std::bitset<kSecureMax> *secure_sessions) {
    return kErrorNone;
  };
  virtual DisplayError SetMixerResolution(uint32_t width, uint32_t height);
  virtual DisplayError GetMixerResolution(uint32_t *width, uint32_t *height);
  virtual uint32_t GetAvailableMixerCount();
  virtual void GetPanelResolution(uint32_t *width, uint32_t *height);
  virtual void GetRealPanelResolution(uint32_t *width, uint32_t *height);
  virtual void Dump(std::ostringstream *os);

  // CWB related methods
  virtual DisplayError GetCwbBufferResolution(CwbConfig *cwb_config,
                                              uint32_t *x_pixels,
                                              uint32_t *y_pixels);
  virtual DisplayError SetReadbackBuffer(void *buffer,
                                         shared_ptr<Fence> acquire_fence,
                                         CwbConfig cwb_config,
                                         CWBClient client);
  virtual CWBReleaseFenceError
  GetReadbackBufferFenceForClient(CWBClient client,
                                  shared_ptr<Fence> *release_fence);
  virtual DisplayError GetReadbackBufferFence(shared_ptr<Fence> *release_fence);
  virtual void ReleaseFrameDumpResources();
  virtual DisplayError TeardownConcurrentWriteback();
  // Captures frame output in the buffer specified by output_buffer_info. The
  // API is non-blocking and the client is expected to check operation status
  // later on. Returns -1 if the input is invalid.
  virtual DisplayError FrameCaptureAsync(const BufferInfo &output_buffer_info,
                                         const CwbConfig &cwb_config) {
    return kErrorNotSupported;
  }
  // Returns the status of frame capture operation requested with
  // FrameCaptureAsync(). kErrorTryAgain : No status obtain yet, call API again
  // after another frame. < 0 : Operation happened but failed. 0 : Success.
  virtual DisplayError GetFrameCaptureStatus() { return kErrorTryAgain; }

  virtual DisplayError SetHWDetailedEnhancerConfig(void *params) {
    return kErrorNotSupported;
  }

  virtual DisplayError SetDisplayDppsAdROI(uint32_t h_start, uint32_t h_end,
                                           uint32_t v_start, uint32_t v_end,
                                           uint32_t factor_in,
                                           uint32_t factor_out) {
    return kErrorNotSupported;
  }
  virtual DisplayError SetFrameTriggerMode(uint32_t mode) {
    return kErrorNotSupported;
  }

  virtual bool IsSmartPanelConfig(uint32_t config_id) { return false; }

  virtual bool HasSmartPanelConfig(void) { return false; }

  virtual bool VsyncEnablePending() { return false; }

  // Display Configurations
  static uint32_t GetThrottlingRefreshRate() {
    return SDMDisplay::throttling_refresh_rate_;
  }
  static void SetThrottlingRefreshRate(uint32_t newRefreshRate) {
    SDMDisplay::throttling_refresh_rate_ = newRefreshRate;
  }
  virtual DisplayError SetNoisePlugInOverride(bool override_en, int32_t attn,
                                              int32_t noise_zpos);
  virtual DisplayError SetActiveDisplayConfig(uint32_t config);
  virtual DisplayError GetActiveDisplayConfig(bool get_real_config, uint32_t *config);
  virtual DisplayError GetDisplayConfigCount(uint32_t *count);
  virtual DisplayError
  GetDisplayAttributesForConfig(int config,
                                DisplayConfigVariableInfo *display_attributes);
  virtual DisplayError GetSupportedDisplayRefreshRates(
      std::vector<uint32_t> *supported_refresh_rates);
  bool IsModeSwitchAllowed(uint32_t config);

  virtual DisplayError Flush() { return kErrorNotSupported; }

  uint32_t GetMaxRefreshRate() { return max_refresh_rate_; }
  DisplayError ToggleScreenUpdates(bool enable);
  DisplayError ColorSVCRequestRoute(const PPDisplayAPIPayload &in_payload,
                                    PPDisplayAPIPayload *out_payload,
                                    PPPendingParams *pending_action);
  void SolidFillPrepare();
  DisplayClass GetDisplayClass();
  DisplayError GetVisibleDisplayRect(SDMRect *rect);
  void BuildLayerStack(void);
  void BuildSolidFillStack(void);
  uint32_t GetGeometryChanges() { return geometry_changes_; }
  SDMColorMode GetCurrentColorMode() {
    return (color_mode_ ? color_mode_->GetCurrentColorMode()
                        : SDMColorMode::COLOR_MODE_SRGB);
  }
  SDMRenderIntent GetCurrentRenderIntent() {
    return (color_mode_ ? color_mode_->GetCurrentRenderIntent()
                        : SDMRenderIntent::COLORIMETRIC);
  }
  bool SDMClientNeedsValidate() {
    return (has_client_composition_ ||
            layer_stack_.flags.single_buffered_layer_present);
  }
  bool CheckResourceState(bool *res_exhausted);
  virtual DisplayError SetColorModeFromClientApi(int32_t color_mode_id) {
    return kErrorNotSupported;
  }
  bool IsFirstCommitDone() { return !first_cycle_; }
  virtual void ProcessActiveConfigChange();

  virtual DisplayError AcceptDisplayChanges(void);
  virtual DisplayError GetActiveConfig(bool get_real_config, Config *out_config);
  virtual DisplayError SetActiveConfig(Config config);
  virtual DisplayError SetPanelLuminanceAttributes(float min_lum,
                                                   float max_lum) {
    return kErrorNotSupported;
  }
  virtual DisplayError
  SetClientTarget(const SnapHandle *target, shared_ptr<Fence> acquire_fence,
                  int32_t dataspace, const SDMRegion &damage, uint32_t version);
  virtual DisplayError GetClientTarget(const SnapHandle *target,
                                       shared_ptr<Fence> acquire_fence,
                                       int32_t dataspace, SDMRegion damage);
  virtual DisplayError SetColorMode(SDMColorMode mode) {
    return kErrorNotSupported;
  }
  virtual DisplayError SetColorModeWithRenderIntent(SDMColorMode mode,
                                                    SDMRenderIntent intent) {
    return kErrorNotSupported;
  }
  virtual DisplayError SetColorModeById(int32_t color_mode_id) {
    return kErrorNotSupported;
  }
  virtual DisplayError RestoreColorTransform() { return kErrorNotSupported; }
  virtual DisplayError SetColorTransform(const float *matrix, SDMColorTransform hint) {
    return kErrorNotSupported;
  }
  virtual DisplayError HandleColorModeTransform(SDMColorMode mode, SDMColorTransform hint,
                                                const double *matrix) {
    return kErrorNotSupported;
  }
  virtual DisplayError SetJitterConfig(uint32_t jitter_type, float value,
                                       uint32_t time) {
    return kErrorNotSupported;
  }
  virtual DisplayError SetBppMode(uint32_t bpp) { return kErrorNotSupported; }
  virtual DisplayError ScheduleDynamicDSIClock(uint64_t bitclk) { return kErrorNotSupported; }

  virtual DisplayError SetDynamicDSIClock() { return kErrorNotSupported; }

  virtual DisplayError GetDynamicDSIClock(uint64_t *bitclk) {
    return kErrorNotSupported;
  }
  virtual DisplayError GetSupportedDSIClock(std::vector<uint64_t> *bitclk) {
    return kErrorNotSupported;
  }
  virtual DisplayError UpdateDisplayId(Display id) {
    return kErrorNotSupported;
  }
  virtual DisplayError SetPendingRefresh() { return kErrorNotSupported; }
  virtual DisplayError SetPanelBrightness(float brightness) {
    return kErrorNotSupported;
  }
  virtual DisplayError GetPanelBrightness(float *brightness) {
    return kErrorNotSupported;
  }
  virtual DisplayError GetPanelMaxBrightness(uint32_t *max_brightness_level) {
    return kErrorNotSupported;
  }
  virtual DisplayError GetDisplayConfigs(std::vector<int32_t> *out_configs);
  DisplayError
  GetAllDisplayAttributes(std::map<uint32_t, DisplayConfigVariableInfo> *info);
  virtual DisplayError GetDisplayAttributes(int32_t config, DisplayConfigVariableInfo *info);
  virtual DisplayError GetClientTargetSupport(int32_t in_width,
                                              int32_t in_height,
                                              LayerBufferFormat format,
                                              Dataspace color_metadata);
  virtual DisplayError GetColorModes(uint32_t *outNumModes,
                                     SDMColorMode *outModes);
  virtual DisplayError GetRenderIntents(SDMColorMode mode,
                                        uint32_t *out_num_intents,
                                        SDMRenderIntent *out_intents);
  virtual DisplayError GetChangedCompositionTypes(uint32_t *out_num_elements,
                                                  LayerId *out_layers,
                                                  int32_t *out_types);
  virtual DisplayError GetDisplayRequests(int32_t *out_display_requests,
                                          uint32_t *out_num_elements,
                                          LayerId *out_layers,
                                          int32_t *out_layer_requests);
  virtual DisplayError GetDisplayName(uint32_t *out_size, char *out_name);
  virtual DisplayError GetDisplayType(int32_t *out_type);
  virtual DisplayError SetCursorPosition(LayerId layer, int x, int y);
  virtual DisplayError SetVsyncEnabled(bool enabled);
  virtual DisplayError SetPowerMode(SDMPowerMode mode, bool teardown);
  virtual DisplayError SetLayerType(LayerId layer_id, SDMLayerTypes type);
  virtual DisplayError
  GetReleaseFences(uint32_t *out_num_elements, LayerId *out_layers,
                   std::vector<shared_ptr<Fence>> *out_fences);
  virtual DisplayError Present(shared_ptr<Fence> *out_retire_fence) = 0;
  virtual DisplayError GetHdrCapabilities(uint32_t *out_num_types,
                                          int32_t *out_types,
                                          float *out_max_luminance,
                                          float *out_max_average_luminance,
                                          float *out_min_luminance);
  virtual DisplayError getDisplayDecorationSupport(uint32_t *format,
                                                   uint32_t *alpha);
  virtual DisplayError SetDisplayAnimating(bool animating);
  virtual bool IsDisplayCommandMode();
  virtual DisplayError SetQSyncMode(QSyncMode qsync_mode) {
    return kErrorNotSupported;
  }
  virtual DisplayError ControlIdlePowerCollapse(bool enable, bool synchronous) {
    return kErrorNone;
  }
  virtual DisplayError GetDisplayIdentificationData(uint8_t *out_port,
                                                    uint32_t *out_data_size,
                                                    uint8_t *out_data);
  virtual DisplayError SetBLScale(uint32_t level) { return kErrorNotSupported; }
  virtual void PostPowerMode();
  virtual SDMPowerMode GetPendingPowerMode() { return pending_power_mode_; }
  virtual void SetPendingPowerMode(SDMPowerMode mode) {
    pending_power_mode_ = mode;
  }
  virtual void ClearPendingPowerMode() {
    pending_power_mode_ = current_power_mode_;
  }
  virtual void NotifyClientStatus(bool connected) {
    client_connected_ = connected;
  }
  virtual DisplayError PostInit() { return kErrorNone; }

  virtual DisplayError
  SetDisplayedContentSamplingEnabledVndService(bool enabled);
  virtual DisplayError
  SetDisplayedContentSamplingEnabled(bool enabled, uint8_t component_mask,
                                     uint64_t max_frames);
  virtual DisplayError
  GetDisplayedContentSamplingAttributes(int32_t *format, int32_t *dataspace,
                                        uint8_t *supported_components);
  virtual DisplayError GetDisplayedContentSample(
      uint64_t max_frames, uint64_t timestamp, uint64_t *numFrames,
      int32_t samples_size[NUM_HISTOGRAM_COLOR_COMPONENTS],
      uint64_t *samples[NUM_HISTOGRAM_COLOR_COMPONENTS]);

  virtual DisplayError GetDisplayVsyncPeriod(bool get_real_config, VsyncPeriodNanos *vsync_period);
  virtual DisplayError SetActiveConfigWithConstraints(
      Config config,
      const SDMVsyncPeriodChangeConstraints *vsync_period_change_constraints,
      SDMVsyncPeriodChangeTimeline *out_timeline);

  DisplayError SetDisplayElapseTime(uint64_t time);
  virtual bool IsDisplayIdle() { return false; };
  virtual bool HasReadBackBufferSupport() { return false; }
  virtual DisplayError NotifyDisplayCalibrationMode(bool in_calibration) {
    return kErrorNotSupported;
  };
  virtual DisplayError CommitOrPrepare(bool validate_only,
                                       shared_ptr<Fence> *out_retire_fence,
                                       uint32_t *out_num_types,
                                       uint32_t *out_num_requests,
                                       bool *needs_commit);
  virtual DisplayError PreValidateDisplay(bool *exit_validate) {
    return kErrorNone;
  }
  DisplayError TryDrawMethod(DisplayDrawMethod client_drawMethod);
  virtual DisplayError SetAlternateDisplayConfig(bool set) {
    return kErrorNotSupported;
  }
  virtual void IsMultiDisplay(bool is_multi_display) {
    is_multi_display_ = is_multi_display;
  }
  virtual DisplayError SetDimmingEnable(int int_enabled) {
    return kErrorNotSupported;
  }
  virtual DisplayError SetDimmingMinBl(int min_bl) {
    return kErrorNotSupported;
  }
  virtual DisplayError RetrieveDemuraTnFiles() { return kErrorNotSupported; }
  virtual DisplayError SetDemuraState(int state) { return kErrorNotSupported; }
  virtual DisplayError SetDemuraConfig(int demura_idx) {
    return kErrorNotSupported;
  }
  virtual DisplayError SetABCState(bool state) { return kErrorNotSupported; }
  virtual DisplayError SetABCReconfig() { return kErrorNotSupported; }
  virtual DisplayError SetABCMode(string mode_name) { return kErrorNotSupported; }
  virtual DisplayError
  GetClientTargetProperty(SDMClientTargetProperty *out_client_target_property);
  virtual void GetConfigInfo(
      std::map<uint32_t, DisplayConfigVariableInfo> *variable_config_map,
      int *active_config_index, uint32_t *num_configs);
  virtual void SetConfigInfo(
      std::map<uint32_t, DisplayConfigVariableInfo> &variable_config_map,
      int active_config_index, uint32_t num_configs){};
  virtual void Abort();
  virtual void MarkClientActive(bool is_client_up);
  virtual void SetExpectedPresentTime(uint64_t time) {
    expected_present_time_ = time;
  }
  virtual DisplayError PerformCacConfig(CacConfig config, bool enable) {
    return kErrorNotSupported;
  }
  virtual DisplayError IsCacV2Supported(bool *supported) {
    *supported = false;
    return kErrorNotSupported;
  }
  int32_t GetDisplayConfigGroup(DisplayConfigGroupInfo variable_config);

  void LayerStackUpdated() {
    layer_stack_invalid_ = true;
  }

  void WaitForDrawCycleToComplete() {
    // ToDo: Replace layer destroy with smart pointer.
    // Work around to block main thread execution until async commit finishes.
    display_intf_->DestroyLayer();
  }
  virtual DisplayError SetupVRRConfig() { return kErrorNotSupported; }
  virtual DisplayError NotifyExpectedPresent(uint64_t expected_present_time,
                                             uint32_t frame_interval_ns);
  virtual void SetFrameIntervalNs(uint32_t fi) { frame_interval_ns_ = fi; }
  virtual DisplayError SetSsrcMode(const std::string &mode) { return kErrorNotSupported; }
  virtual DisplayError EnableCopr(bool en) { return kErrorNotSupported; }
  virtual DisplayError GetCoprStats(std::vector<int> *stats) { return kErrorNotSupported; }
  virtual int GetNotifyEptConfig() { return -1; }
  virtual DisplayError SetPanelFeatureConfig(int32_t type, void *data) {
    return kErrorNotSupported;
  }

 protected:
  static uint32_t throttling_refresh_rate_;
  // Maximum number of layers supported by display manager.
  static const uint32_t kMaxLayerCount = 32;
  static bool mmrm_restricted_;
  SDMDisplay(CoreInterface *core_intf, BufferAllocator *buffer_allocator,
             SDMCompositorCallbacks *callbacks, SDMDisplayEventHandler *event_handler,
             SDMDisplayType type, Display id, int32_t sdm_id, DisplayClass display_class);

  // DisplayEventHandler methods
  virtual DisplayError VSync(const DisplayEventVSync &vsync);
  virtual DisplayError Refresh();
  virtual DisplayError CECMessage(char *message);
  virtual DisplayError HistogramEvent(int source_fd, uint32_t blob_id);
  virtual DisplayError HandleEvent(DisplayEvent event);
  virtual DisplayError HandleQsyncState(const QsyncEventData &qsync_data);
  virtual void NotifyCwbDone(int32_t status, const LayerBuffer &buffer);
#ifdef SEC_GC_QC_DYN_CLK
  virtual void NotifyDynamicDSIClock(uint64_t bitclk);
#endif
  virtual void DumpOutputBuffer(const BufferInfo &buffer_info, void *base,
                                shared_ptr<Fence> &retire_fence);
  virtual DisplayError PrepareLayerStack(uint32_t *out_num_types,
                                         uint32_t *out_num_requests);
  virtual DisplayError CommitLayerStack(void);
  virtual DisplayError
  PostCommitLayerStack(shared_ptr<Fence> *out_retire_fence);
  virtual DisplayError DisablePartialUpdateOneFrame() {
    return kErrorNotSupported;
  }
  virtual void ReqPerfHintRelease() { return; }
  const char *GetDisplayString();
  void MarkLayersForGPUBypass(void);
  void MarkLayersForClientComposition(void);
  void UpdateConfigs();
  virtual void ApplyScanAdjustment(SDMRect *display_frame);
  bool IsLayerUpdating(SDMLayer *layer);
  uint32_t SanitizeRefreshRate(uint32_t req_refresh_rate);
  virtual void GetUnderScanConfig() {}
  int32_t SetClientTargetDataSpace(int32_t dataspace);
  DisplayError SetFrameBufferConfig(uint32_t x_pixels, uint32_t y_pixels);
  DisplayError GetVsyncPeriodByActiveConfig(bool get_real_config, VsyncPeriodNanos *vsync_period);
  bool GetTransientVsyncPeriod(VsyncPeriodNanos *vsync_period);
  std::tuple<int64_t, int64_t>
  RequestActiveConfigChange(Config config,
                            VsyncPeriodNanos current_vsync_period,
                            int64_t desired_time);
  std::tuple<int64_t, int64_t>
  EstimateVsyncPeriodChangeTimeline(VsyncPeriodNanos current_vsync_period,
                                    int64_t desired_time);
  void SubmitActiveConfigChange(VsyncPeriodNanos current_vsync_period);
  bool IsActiveConfigReadyToSubmit(int64_t time);
  bool IsActiveConfigApplied(int64_t time, int64_t vsync_applied_time);
  bool IsSameGroup(Config config_id1, Config config_id2);
  bool AllowSeamless(Config request_config);
  void SetVsyncsApplyRateChange(uint32_t vsyncs) {
    vsyncs_to_apply_rate_change_ = vsyncs;
  }
  DisplayError SubmitDisplayConfig(Config config);
  DisplayError GetCachedActiveConfig(bool get_real_config, Config *config);
  void SetActiveConfigIndex(int active_config_index);
  DisplayError PostPrepareLayerStack(uint32_t *out_num_types,
                                     uint32_t *out_num_requests);
  DisplayError HandlePrepareError(DisplayError error);
  int GetActiveConfigIndex();
  DisplayError ValidateTUITransition(SecureEvent secure_event);
  void MMRMEvent(bool restricted);
  void UpdateRefreshRate();
  void UpdateActiveConfig();
  void DumpInputBuffers(void);
  void RetrieveFences(shared_ptr<Fence> *out_retire_fence);
  void SetDrawMethod();

  // CWB related methods
  void HandleFrameOutput();
  void HandleFrameDump();
  virtual void HandleFrameCapture(){};

  std::shared_ptr<ISnapMapper> snapmapper_;
  std::shared_ptr<SDMLayerBuilder> layer_builder_;
  bool layer_stack_invalid_ = true;
  CoreInterface *core_intf_ = nullptr;
  BufferAllocator *buffer_allocator_ = NULL;
  SDMCompositorCallbacks *callbacks_ = nullptr;
  SDMDisplayEventHandler *event_handler_ = nullptr;
  SDMDisplayType type_ = kDisplayTypeMax;
  Display id_ = UINT64_MAX;
  int32_t sdm_id_ = -1;
  DisplayInterface *display_intf_ = NULL;
  LayerStack layer_stack_;
  SDMLayer *client_target_ = nullptr; // Also known as framebuffer target

  std::map<LayerId, SDMCompositionType> layer_changes_;
  std::map<LayerId, SDMLayerRequest> layer_requests_;
  bool flush_on_error_ = false;
  bool flush_ = false;
  SDMPowerMode current_power_mode_ = SDMPowerMode::POWER_MODE_OFF;
  SDMPowerMode pending_power_mode_ = SDMPowerMode::POWER_MODE_OFF;
  bool swap_interval_zero_ = false;
  bool display_paused_ = false;
  uint32_t min_refresh_rate_ = 0;
  uint32_t max_refresh_rate_ = 0;
  uint32_t qsync_fps_ = 0;
  uint32_t current_refresh_rate_ = 0;
  bool use_metadata_refresh_rate_ = false;
  bool boot_animation_completed_ = false;
  bool shutdown_pending_ = false;
  std::bitset<kSecureMax> active_secure_sessions_ = 0;
  bool solid_fill_enable_ = false;
  Layer *solid_fill_layer_ = NULL;
  LayerRect solid_fill_rect_ = {};
  LayerSolidFill solid_fill_color_ = {};
  LayerRect display_rect_;
  bool color_tranform_failed_ = false;
  SDMColorModeMgr *color_mode_ = NULL;
  uint32_t num_configs_ = 0;
  int disable_hdr_handling_ = 0;  // disables HDR handling.
  int disable_sdr_histogram_ = 0; // disables handling of SDR histogram data.
  bool pending_commit_ = false;
  bool is_cmd_mode_ = false;
  bool partial_update_enabled_ = false;
  bool skip_commit_ = false;
  std::map<uint32_t, DisplayConfigVariableInfo> variable_config_map_;
  std::vector<uint32_t> sdm_config_map_;
  bool client_connected_ = true;
  bool pending_config_ = false;
  bool has_client_composition_ = false;
  LayerRect window_rect_ = {};
  bool windowed_display_ = false;
  uint32_t vsyncs_to_apply_rate_change_ = 1;
  Config pending_refresh_rate_config_ = UINT_MAX;
  int64_t pending_refresh_rate_refresh_time_ = INT64_MAX;
  int64_t pending_refresh_rate_applied_time_ = INT64_MAX;
  std::deque<TransientRefreshRateInfo> transient_refresh_rate_info_;
  std::mutex transient_refresh_rate_lock_;
  std::mutex active_config_lock_;
  std::mutex frame_dump_config_lock_;
  int active_config_index_ = -1;
  uint32_t active_refresh_rate_ = 0;
  SecureEvent secure_event_ = kSecureEventMax;
  bool display_pause_pending_ = false;
  bool display_idle_ = false;
  bool animating_ = false;
  DisplayDrawMethod draw_method_ = kDrawDefault;
  uint32_t fb_width_ = 0;
  uint32_t fb_height_ = 0;

  // Members for N frame dump to file
  bool dump_output_to_file_ = false;
  uint32_t dump_frame_count_ = 0; // tracks output frames count which to be dump
  uint32_t dump_frame_index_ =
      0; // tracks current output frame index which to be dump
  uint32_t dump_input_frame_count_ =
      0; // tracks input frames count which to be dump
  uint32_t dump_input_frame_index_ =
      0; // tracks current input frame index which to be dump
  bool dump_input_layers_ = false;
  BufferInfo output_buffer_info_ = {};
  void *output_buffer_base_ =
      nullptr; // points to base address of output_buffer_info_
  CwbConfig output_buffer_cwb_config_ = {};

  // Members for 1 frame capture in a client provided buffer
  bool frame_capture_buffer_queued_ = false;
  DisplayError frame_capture_status_ = kErrorTryAgain;
  uint32_t geometry_changes_ = GeometryChanges::kNone;
  bool is_multi_display_ = false;
  const SnapHandle *client_target_handle_;
  shared_ptr<Fence> client_acquire_fence_ = nullptr;
  int32_t client_dataspace_ = 0;
  SDMRegion client_damage_region_ = {};
  std::map<uint64_t, CWBClient> cwb_buffer_map_ = {};
  std::mutex cwb_mutex_;
  std::condition_variable cwb_cv_;
  std::map<CWBClient, CWBCaptureResponse> cwb_capture_status_map_;
  static constexpr unsigned int kCwbWaitMs = 100;
  bool validate_done_ = false;
  SDMLayerStack *sdm_layer_stack_ = nullptr;
  uint64_t scheduled_dynamic_dsi_clk_ = 0;

private:
  bool CanSkipSdmPrepare(uint32_t *num_types, uint32_t *num_requests);
  void WaitOnPreviousFence();
  bool IsPanelConfig(uint32_t x, uint32_t y);
  void PopulateSDMExtendedDisplayResolution();
  DisplayError GetSDMActiveConfig(bool get_real_config, Config *config_index);
  bool IsVirtualConfig(Config config);
  DisplayError SetFBForExtendedResolution(Config config, bool *is_virtual_config_fps_switched);
  DisplayError FinalizeDisplayConfig(bool check_pending_config, Config new_config);
  DisplayError GetParentConfig(Config *config);
  bool NotifyIdleNow();

  DisplayClass display_class_;
  uint32_t geometry_changes_on_doze_suspend_ = GeometryChanges::kNone;
  bool first_cycle_ =
      true; // false if a display commit has succeeded on the device.
  shared_ptr<Fence> release_fence_ = nullptr;
  Config pending_config_index_ = 0;
  bool pending_first_commit_config_ = false;
  Config pending_first_commit_config_index_ = 0;
  bool game_supported_ = false;
  uint64_t elapse_timestamp_ = 0;
  bool draw_method_set_ = false;
  bool client_target_3_1_set_ = false;
  bool is_client_up_ = false;
  uint64_t expected_present_time_ = 0;  // Expected Present time for current frame
  int idle_active_ms_ = 0;
  uint32_t frame_interval_ns_ = 0;  // FrameInterval for current frame
};

inline DisplayError SDMDisplay::Perform(uint32_t operation, ...) {
  return kErrorNone;
}

} // namespace sdm

#endif // __SDM_DISPLAY_H__
