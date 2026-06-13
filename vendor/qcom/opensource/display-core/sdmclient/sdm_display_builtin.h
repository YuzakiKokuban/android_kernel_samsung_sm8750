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
#ifndef __SDM_DISPLAY_BUILTIN_H__
#define __SDM_DISPLAY_BUILTIN_H__

#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "cpuhint.h"
#include "sdm_display.h"
#include "sdm_layers.h"
#include "utils/constants.h"
#include "utils/sync_task.h"

namespace sdm {

struct LayerStitchGetInstanceContext
    : public SyncTask<LayerStitchTaskCode>::TaskContext {
  LayerBuffer *output_buffer = NULL;
};

class SDMDisplayBuiltIn : public SDMDisplay,
                          public SyncTask<LayerStitchTaskCode>::TaskHandler {
public:
 static DisplayError Create(CoreInterface *core_intf, BufferAllocator *buffer_allocator,
                            SDMCompositorCallbacks *callbacks,
                            SDMDisplayEventHandler *event_handler, Display id, int32_t sdm_id,
                            SDMDisplay **sdm_display);
 static void Destroy(SDMDisplay *sdm_display);
 virtual DisplayError Init();
 virtual DisplayError Present(shared_ptr<Fence> *out_retire_fence);
 virtual DisplayError CommitLayerStack();
 virtual DisplayError GetColorModes(uint32_t *out_num_modes, SDMColorMode *out_modes);
 virtual DisplayError SetColorMode(SDMColorMode mode);
 virtual DisplayError GetRenderIntents(SDMColorMode mode, uint32_t *out_num_intents,
                                       SDMRenderIntent *out_intents);
 virtual DisplayError SetColorModeWithRenderIntent(SDMColorMode mode, SDMRenderIntent intent);
 virtual DisplayError SetColorModeById(int32_t color_mode_id);
 virtual DisplayError SetColorModeFromClientApi(int32_t color_mode_id);
 virtual DisplayError SetColorTransform(const float *matrix, SDMColorTransform hint);
 virtual DisplayError RestoreColorTransform();
 virtual DisplayError Perform(uint32_t operation, ...);
 virtual DisplayError GetActiveSecureSession(std::bitset<kSecureMax> *secure_sessions);
 virtual DisplayError HandleSecureSession(const std::bitset<kSecureMax> &secure_session,
                                          bool *power_on_pending, bool is_active_secure_display);
 virtual void SetIdleTimeoutMs(uint32_t timeout_ms, uint32_t inactive_ms);
 virtual DisplayError FrameCaptureAsync(const BufferInfo &output_buffer_info,
                                        const CwbConfig &cwb_config);
 virtual DisplayError GetFrameCaptureStatus() { return frame_capture_status_; }
 virtual DisplayError SetDetailEnhancerConfig(const DisplayDetailEnhancerData &de_data);
 virtual DisplayError SetHWDetailedEnhancerConfig(void *params);
 virtual DisplayError ControlPartialUpdate(bool enable, uint32_t *pending);
 virtual DisplayError SetBppMode(uint32_t bpp);
 virtual DisplayError SetQSyncMode(QSyncMode qsync_mode);
 virtual DisplayError ControlIdlePowerCollapse(bool enable, bool synchronous);
 virtual DisplayError SetDisplayDppsAdROI(uint32_t h_start, uint32_t h_end, uint32_t v_start,
                                          uint32_t v_end, uint32_t factor_in, uint32_t factor_out);
 virtual DisplayError SetJitterConfig(uint32_t jitter_type, float value, uint32_t time);
 virtual DisplayError SetDynamicDSIClock();
 virtual DisplayError ScheduleDynamicDSIClock(uint64_t bitclk);
 virtual DisplayError GetDynamicDSIClock(uint64_t *bitclk);
 virtual DisplayError GetSupportedDSIClock(std::vector<uint64_t> *bitclk_rates);
 virtual DisplayError UpdateDisplayId(Display id);
 virtual DisplayError SetPendingRefresh();
 virtual DisplayError SetPanelBrightness(float brightness);
 virtual DisplayError GetPanelBrightness(float *brightness);
 virtual DisplayError GetPanelMaxBrightness(uint32_t *max_brightness_level);
 virtual DisplayError SetFrameTriggerMode(uint32_t mode);
 virtual DisplayError SetBLScale(uint32_t level);
 virtual DisplayError SetClientTarget(const SnapHandle *target, shared_ptr<Fence> acquire_fence,
                                      int32_t dataspace, const SDMRegion &damage, uint32_t version);
 virtual bool IsSmartPanelConfig(uint32_t config_id);
 virtual bool HasSmartPanelConfig(void);
 virtual DisplayError Deinit();
 virtual DisplayError PostInit();

 virtual DisplayError SetDisplayedContentSamplingEnabledVndService(bool enabled);
 virtual DisplayError SetDisplayedContentSamplingEnabled(bool enabled, uint8_t component_mask,
                                                         uint64_t max_frames);
 virtual DisplayError GetDisplayedContentSamplingAttributes(int32_t *format, int32_t *dataspace,
                                                            uint8_t *supported_components);
 virtual DisplayError GetDisplayedContentSample(
     uint64_t max_frames, uint64_t timestamp, uint64_t *numFrames,
     int32_t samples_size[NUM_HISTOGRAM_COLOR_COMPONENTS],
     uint64_t *samples[NUM_HISTOGRAM_COLOR_COMPONENTS]);
 void Dump(std::ostringstream *os) override;
 virtual DisplayError SetPowerMode(SDMPowerMode mode, bool teardown);
 virtual bool IsDisplayIdle();
 virtual bool HasReadBackBufferSupport();
 virtual DisplayError NotifyDisplayCalibrationMode(bool in_calibration);
 virtual DisplayError CommitOrPrepare(bool validate_only, shared_ptr<Fence> *out_retire_fence,
                                      uint32_t *out_num_types, uint32_t *out_num_requests,
                                      bool *needs_commit);
 virtual DisplayError PreValidateDisplay(bool *exit_validate);
 virtual DisplayError PostCommitLayerStack(shared_ptr<Fence> *out_retire_fence);
 virtual DisplayError SetAlternateDisplayConfig(bool set);
 virtual DisplayError SetDimmingEnable(int int_enabled);
 virtual DisplayError SetDimmingMinBl(int min_bl);
 virtual DisplayError RetrieveDemuraTnFiles();
 virtual DisplayError UpdateTransferTime(uint32_t transfer_time);
 virtual DisplayError SetDemuraState(int state);
 virtual DisplayError SetDemuraConfig(int demura_idx);
 virtual DisplayError PerformCacConfig(CacConfig config, bool enable);
 virtual DisplayError IsCacV2Supported(bool *supported);
 virtual DisplayError SetSsrcMode(const std::string &mode);
 virtual DisplayError EnableCopr(bool en);
 virtual DisplayError GetCoprStats(std::vector<int> *stats);
 virtual DisplayError SetupVRRConfig();
 virtual int GetNotifyEptConfig();
 virtual DisplayError SetABCState(bool state);
 virtual DisplayError SetABCReconfig();
 virtual DisplayError SetABCMode(string mode_name);
 virtual DisplayError SetPanelFeatureConfig(int32_t type, void *data);

private:
 SDMDisplayBuiltIn(CoreInterface *core_intf, BufferAllocator *buffer_allocator,
                   SDMCompositorCallbacks *callbacks, SDMDisplayEventHandler *event_handler,
                   Display id, int32_t sdm_id);
 void SetMetaDataRefreshRateFlag(bool enable);
 virtual DisplayError SetDisplayMode(uint32_t mode);
 virtual DisplayError DisablePartialUpdateOneFrame();
 void ProcessBootAnimCompleted(void);
 void SetQDCMSolidFillInfo(bool enable, const LayerSolidFill &color);
 void ForceRefreshRate(uint32_t refresh_rate);
 uint32_t GetOptimalRefreshRate(bool one_updating_layer);
 virtual void HandleFrameCapture();
 bool CanSkipCommit();
 DisplayError SetMixerResolution(uint32_t width, uint32_t height);
 DisplayError GetMixerResolution(uint32_t *width, uint32_t *height);
 DisplayError CommitStitchLayers();
 void AppendStitchLayer();
 bool InitLayerStitch();
 void InitStitchTarget();
 bool AllocateStitchBuffer();
 void PostCommitStitchLayers();
 bool NeedsLargeCompPerfHint();
 void ValidateUiScaling();
 void EnablePartialUpdate();
 uint32_t GetUpdatingAppLayersCount();
 void LoadMixedModePerfHintThreshold();
 void HandleLargeCompositionHint(bool release);
 void ReqPerfHintRelease();

 // SyncTask methods.
 void OnTask(const LayerStitchTaskCode &task_code,
             SyncTask<LayerStitchTaskCode>::TaskContext *task_context);

 const int kPerfHintLargeCompCycle = 0x00001097;
 const int kPerfHintDisplayOff = 0x00001040;
 const int kPerfHintDisplayOn = 0x00001041;
 const int kPerfHintDisplayDoze = 0x00001053;
 BufferAllocator *buffer_allocator_ = nullptr;
 CPUHint *cpu_hint_ = nullptr;

 bool pending_refresh_ = true;
 bool enable_optimize_refresh_ = false;
 bool enable_poms_during_doze_ = false;

 bool is_primary_ = false;
 bool disable_layer_stitch_ = true;
 SDMLayer *stitch_target_ = nullptr;
 SyncTask<LayerStitchTaskCode> layer_stitch_task_;
 BufferInfo buffer_info_ = {};
 DisplayConfigVariableInfo fb_config_ = {};

 bool qsync_enabled_ = false;
 bool qsync_reconfigured_ = false;
 // Members for Color sampling feature
 DisplayError HistogramEvent(int fd, uint32_t blob_id) override;
 std::mutex sampling_mutex;
 bool api_sampling_vote = false;
 bool vndservice_sampling_vote = false;

 int perf_hint_large_comp_cycle_ = 0;
 bool force_reset_lut_ = false;
 bool disable_dyn_fps_ = false;
 bool enable_round_corner_ = false;
 bool enhance_idle_time_ = false;
 shared_ptr<Fence> retire_fence_ = nullptr;
 std::unordered_map<int32_t, int32_t> mixed_mode_threshold_;
 int alternate_config_ = -1;

 // Long term large composition hint
 int sdm_tid_ = 0;
 uint32_t large_comp_hint_threshold_ = 0;
 nsecs_t hint_release_start_time_ = 0;
 nsecs_t elapse_time_threshold_ = 100;  // Time is in milliseconds

 // Nominal VSync multiplier for Notify EPT heads-up
 const int32_t notify_ept_heads_up_config_ = 2;

 // Commit counter for dynamic dsi clock
 bool commit_counter_ = false;
};

} // namespace sdm

#endif // __SDM_DISPLAY_BUILTIN_H__
